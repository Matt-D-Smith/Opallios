cd /home/debian/raylib/src
make PLATFORM=PLATFORM_DESKTOP GRAPHICS=GRAPHICS_API_OPENGL_ES2
make install
cp libraylib.a /home/debian/Opallios/sw/opallios
cd /home/debian/Opallios/sw/opallios
