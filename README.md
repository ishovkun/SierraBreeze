Special thanks to:
- Chris Holland for his blog post
https://zren.github.io/projects/2017/07/08/patching-breeze-window-decorations.html

- The author of Breezemite Andrey Orst

Compile with
'''
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_LIBDIR=lib -DBUILD_TESTING=OFF -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
sudo make install
kwin_x11 --replace & disown
'''

