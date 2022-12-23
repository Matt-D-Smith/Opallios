cd /home/debian/raylib/src
make PLATFORM=PLATFORM_DRM GRAPHICS=GRAPHICS_API_OPENGL_ES2
make install
cp libraylib.a /home/debian/Opallios/sw/opallios
cd /home/debian/Opallios/sw/opallios
