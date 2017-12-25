# SierraBreeze
Sierra Breeze is a fork of Breeze Window decoration that aims to mimic the
OSX Sierra titlebar buttons. 
It looks something like this:
![Screenshot](Screenshot.png)

## Dependencies
There are some dependencies you'll need to install. Some people suggested using the following commands:
### Ubuntu
``` shell
sudo apt install libkf5config-dev libkdecorations2-dev libqt5x11extras5-dev qtdeclarative5-dev
```

### Arch Linux
``` shell
sudo pacman -S kdecoration qt5-declarative qt5-x11extras    # Decoration
sudo pacman -S cmake extra-cmake-modules                    # Installation
```
SierraBreeze can also be installed from the [AUR](https://aur.archlinux.org/packages/sierrabreeze-kwin-decoration-git/) using
``` shell
yaourt errabreeze-kwin-decoration-git
```

## Installation
In order to install the theme and add it to your decorations do the following:
``` shell
git clone https://github.com/ishovkun/SierraBreeze
cd SierraBreeze
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_LIBDIR=lib -DBUILD_TESTING=OFF -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
sudo make install
```
To avoid logging out, issue
``` shell
kwin_x11 --replace &
```
That is it! Your new decoration theme should appear in
*Settings &rarr; Application Style &rarr; Window Decorations*.

## Acknowledgments:
- The authors of Breeze window decorations Martin Gräßlin and Hugo Pereira Da Costa
- Andrey Orst, the author of Breezemite Aurorae window decoration
https://github.com/andreyorst/Breezemite
- Chris Holland for his blog about patching Breeze decorations
https://zren.github.io/projects/2017/07/08/patching-breeze-window-decorations.html
