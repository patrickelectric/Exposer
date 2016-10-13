# Exposer [![Build Status](https://travis-ci.org/robotadasufsc/Exposer.svg?branch=dev)](https://travis-ci.org/Williangalvani/Exposer)
---
An Arduino Library to enable changing variables via serial communication.

# How to use:
- Include the library
 
```#include "exposer.h"````
- Instantiate the Exposer:
 
```Exposer* exposer = &Exposer::self();```

- Register any variables you wish to expose:

```exposer->registerVariable("Variable", Exposer::_uint8_t, &Variable);```

This exposes Variable, and tells the interface it's a uint8_t type, named "Variable" on the interface.

Now a serial interface is able to view and edit this variable.
