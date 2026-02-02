# xinitrc file for testing our window manager.

# 1. Start some programs to play with.
sleep 2 && ./esde.AppImage --resolution 1280 720 &

# 2. Start our window manager.
exec ./basic_wm
