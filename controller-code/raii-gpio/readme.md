# Utility classes
## C++ GPIO read/write wrapper
Standalone header to allow simple read/write operations to GPIO pins without having
    to worry about handling specific errors when opening, etc.
It's [RAII](https://en.cppreference.com/w/cpp/language/raii)!

### Prerequesite packages
You need the `gpiod` packages:
```sh
sudo apt install gpiod libgpiod-dev -y
```

### Example program is in `tests`
