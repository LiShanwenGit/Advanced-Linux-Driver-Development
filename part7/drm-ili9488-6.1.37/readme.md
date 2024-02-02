## How to use it  
add device tree node   
```
&spi1 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&spi1_pins>, <&spi1_cs0_pin>;
    ili9488@0 {
        compatible = "ilitek,ili9488";
        reg = <0>;
        spi-cpol;
        spi-cpha;
        width = <480>;
        height = <320>;
        width-mm = <83>;
        height-mm = <55>;
        spi-max-frequency = <50000000>;
        reset-gpios = <&pio 2 5 GPIO_ACTIVE_HIGH>;  //PC5
        dc-gpios = <&pio 2 8 GPIO_ACTIVE_HIGH>;     //PC8
    };
};

```
# 3 ways for it   
# 1. no parameter, it will use device tree, if width and height are both invalid in device tree, it will use default:480x320
```shell
insmod drm-ili9488.ko 
```
# 2. parameter, hd=480 vd=320 hd_mm=83 vd_mm=55
```shell
insmod drm-st7789.ko hd=480 vd=320 hd_mm=83 vd_mm=55
```
# 3. device tree
add propertys into yor spi device node, such as:
```
width = <480>;
height = <320>;
width-mm = <83>;
height-mm = <55>;
```