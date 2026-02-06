# xinitrc file for testing our window manager.

# 1. Start some programs to play with.
sleep 2 && ./esde.AppImage &

# 2. Start our window manager.
exec ./basic_wm
