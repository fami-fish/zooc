#include "screenshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>

#ifdef MITSHM
enum {
    SHMGET_NUM = 29,
    SHMAT_NUM = 30,
    SHMCTL_NUM = 31,
    SHMDT_NUM = 67
};

static inline long syscall_shmget(key_t key, size_t size, int shmflg) {
    long ret;
    asm volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SHMGET_NUM), "D"(key), "S"(size), "d"(shmflg)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_shmat(int shmid, const void *shmaddr, int shmflg) {
    long ret;
    asm volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SHMAT_NUM), "D"(shmid), "S"(shmaddr), "d"(shmflg)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_shmctl(int shmid, int cmd, struct shmid_ds *buf) {
    long ret;
    asm volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SHMCTL_NUM), "D"(shmid), "S"(cmd), "d"(buf)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static inline long syscall_shmdt(const void *shmaddr) {
    long ret;
    asm volatile (
        "syscall"
        : "=a"(ret)
        : "a"(SHMDT_NUM), "D"(shmaddr)
        : "rcx", "r11", "memory"
    );
    return ret;
}
#endif

Screenshot new_screenshot(Display *display, Window window) {
    Screenshot result = {0};
    XWindowAttributes attributes;
    
    if (!XGetWindowAttributes(display, window, &attributes)) {
        fprintf(stderr, "Failed to get window attributes\n");
        return result;
    }
    
#ifdef MITSHM
    result.shminfo = malloc(sizeof(XShmSegmentInfo));
    if (!result.shminfo) {
        fprintf(stderr, "Failed to allocate memory for shminfo\n");
        return result;
    }
    
    int screen = DefaultScreen(display);
    result.image = XShmCreateImage(
        display,
        DefaultVisual(display, screen),
        DefaultDepth(display, screen),
        ZPixmap,
        NULL,
        result.shminfo,
        attributes.width,
        attributes.height
    );
    
    if (!result.image) {
        fprintf(stderr, "Failed to create XShmImage\n");
        return result;
    }
    
    result.shminfo->shmid = syscall_shmget(
        IPC_PRIVATE,
        result.image->bytes_per_line * result.image->height,
        IPC_CREAT | 0777
    );
    
    if (result.shminfo->shmid < 0) {
        fprintf(stderr, "Failed to allocate shared memory segment\n");
        XDestroyImage(result.image);
        return result;
    }
    
    result.shminfo->shmaddr = (char *)syscall_shmat(
        result.shminfo->shmid,
        0,
        0
    );
    
    if ((long)result.shminfo->shmaddr == -1) {
        fprintf(stderr, "Failed to attach shared memory\n");
        syscall_shmctl(result.shminfo->shmid, IPC_RMID, NULL);
        XDestroyImage(result.image);
        return result;
    }
    
    result.image->data = result.shminfo->shmaddr;
    result.shminfo->readOnly = 0;
    
    if (!XShmAttach(display, result.shminfo)) {
        fprintf(stderr, "Failed to attach shared memory to X server\n");
        syscall_shmdt(result.shminfo->shmaddr);
        syscall_shmctl(result.shminfo->shmid, IPC_RMID, NULL);
        XDestroyImage(result.image);
        return result;
    }
    
    XSync(display, False);
    
    if (!XShmGetImage(display, window, result.image, 0, 0, AllPlanes)) {
        fprintf(stderr, "Failed to get image via MIT-SHM\n");
        XShmDetach(display, result.shminfo);
        syscall_shmdt(result.shminfo->shmaddr);
        syscall_shmctl(result.shminfo->shmid, IPC_RMID, NULL);
        XDestroyImage(result.image);
        free(result.shminfo);
        result.image = NULL;
    }
#else
    result.image = XGetImage(
        display,
        window,
        0,
        0,
        attributes.width,
        attributes.height,
        AllPlanes,
        ZPixmap
    );
    
    if (!result.image) {
        fprintf(stderr, "Failed to get XImage\n");
    }
#endif
    
    return result;
}

void destroy_screenshot(Screenshot screenshot, Display *display) {
    if (!screenshot.image) return;
    
#ifdef MITSHM
    if (screenshot.shminfo) {
        XSync(display, False);
        XShmDetach(display, screenshot.shminfo);
        syscall_shmdt(screenshot.shminfo->shmaddr);
        syscall_shmctl(screenshot.shminfo->shmid, IPC_RMID, NULL);
    }
#endif
    
    XDestroyImage(screenshot.image);
    
#ifdef MITSHM
    if (screenshot.shminfo) {
        free(screenshot.shminfo);
    }
#endif
}

void refresh_screenshot(Screenshot *screenshot, Display *display, Window window) {
    if (!screenshot || !screenshot->image) {
        *screenshot = new_screenshot(display, window);
        return;
    }
    
    XWindowAttributes attributes;
    if (!XGetWindowAttributes(display, window, &attributes)) {
        fprintf(stderr, "Failed to get window attributes during refresh\n");
        return;
    }
    
#ifdef MITSHM
    if (!XShmGetImage(display, window, screenshot->image, 0, 0, AllPlanes) ||
        attributes.width != screenshot->image->width ||
        attributes.height != screenshot->image->height) {
        destroy_screenshot(*screenshot, display);
        *screenshot = new_screenshot(display, window);
    }
#else
    XImage *refreshedImage = XGetSubImage(
        display,
        window,
        0,
        0,
        screenshot->image->width,
        screenshot->image->height,
        AllPlanes,
        ZPixmap,
        screenshot->image,
        0,
        0
    );
    
    if (!refreshedImage ||
        refreshedImage->width != attributes.width ||
        refreshedImage->height != attributes.height) {
        XImage *newImage = XGetImage(
            display,
            window,
            0,
            0,
            attributes.width,
            attributes.height,
            AllPlanes,
            ZPixmap
        );
        
        if (newImage) {
            XDestroyImage(screenshot->image);
            screenshot->image = newImage;
        } else if (refreshedImage) {
            // Keep the refreshed image if new one failed
            if (screenshot->image != refreshedImage) {
                XDestroyImage(screenshot->image);
            }
            screenshot->image = refreshedImage;
        }
    } else {
        if (screenshot->image != refreshedImage) {
            XDestroyImage(screenshot->image);
        }
        screenshot->image = refreshedImage;
    }
#endif
}

void save_to_ppm(XImage *image, const char *filePath) {
    if (!image || !image->data) {
        fprintf(stderr, "Invalid image for saving\n");
        return;
    }
    
    FILE *f = fopen(filePath, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open file %s for writing\n", filePath);
        return;
    }
    
    fprintf(f, "P6\n%d %d\n255\n", image->width, image->height);
    
    int bytesPerPixel = image->bits_per_pixel / 8;
    if (bytesPerPixel < 3) {
        fprintf(stderr, "Unsupported bits per pixel: %d\n", image->bits_per_pixel);
        fclose(f);
        return;
    }
    
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            
            unsigned char r = (pixel >> 16) & 0xFF;
            unsigned char g = (pixel >> 8) & 0xFF;
            unsigned char b = pixel & 0xFF;
            
            fputc(r, f);
            fputc(g, f);
            fputc(b, f);
        }
    }
    
    fclose(f);
}

void cleanupScreenshot(Screenshot *screenshot) {
    if (!screenshot) return;
    
#ifdef MITSHM
    if (screenshot->shminfo) {
        free(screenshot->shminfo);
        screenshot->shminfo = NULL;
    }
#endif
    screenshot->image = NULL;
}
