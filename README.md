# SierraBreeze
Sierra Breeze is a fork of Breeze Window decoration that aims to mimic the
OSX Sierra titlebar buttons. It looks something like this:

## Installation
There are some dependencies you'll need to install. Some people suggested using the following
command on Ubuntu:
```
sudo apt install libkf5config-dev libkdecorations2-dev libqt5x11extras5-dev qtdeclarative5-dev
```
Then do the following:
```
git clone sierrabreeze
cd sierrabreeze
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_LIBDIR=lib -DBUILD_TESTING=OFF -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
sudo make install
```
In order to avoid logging out, issue
```
kwin_x11 --replace & disown
```
That is it!

Acknowledgments:
- The authors of Breeze window decorations Martin Gräßlin and Hugo Pereira Da Costa
- Andrey Orst, the author of Breezemite Aurorae window decoration
https://github.com/andreyorst/Breezemite
- Chris Holland for his blog about patching Breeze decorations
https://zren.github.io/projects/2017/07/08/patching-breeze-window-decorations.html