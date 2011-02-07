gcc -c fuzzy_plugin.c -fPIC `pkg-config --cflags geany` 
gcc fuzzy_plugin.o -o fuzzy_plugin.so -shared `pkg-config --libs geany` 
sudo cp fuzzy_plugin.so /usr/lib/geany
