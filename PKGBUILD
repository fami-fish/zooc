# Maintainer: Nicholas Bors-Sterian <fami_fish@proton.me>
pkgname=zooc
pkgver=1.0.0
pkgrel=1
pkgdesc="A C re-write of tsoding's boomer with some added features"
arch=('x86_64')
url="https://github.com/fami-fish/${pkgname}.git"
license=('MIT')
depends=('glibc' 'glew')
makedepends=('git' 'make' 'gcc')
source=("https://github.com/fami-fish/${pkgname}/releases/${pkgver}/zooc.tar.gz")
sha256sums=('SKIP')

prepare() {
    cd "$srcdir"
}

build() {
    cd "$srcdir/zooc-${pkgver}"
    make
}

package() {
    cd "$srcdir/zooc-${pkgver}"
    make DESTDIR="$pkgdir" install
}
