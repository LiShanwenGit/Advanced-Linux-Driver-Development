## How to use it  
add device tree node   
```
&spi1 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&spi1_pins>, <&spi1_cs_pin>;
    st7789@0 {
        compatible = "sitronix,st7789";
        reg = <0>;
        spi-cpol;
        spi-cpha;
        width = <240>;
        height = <240>;
        width-mm = <23>;
        height-mm = <23>;
        spi-max-frequency = <40000000>;
        reset-gpios = <&pio 2 15 GPIO_ACTIVE_HIGH>;  //PC15
        dc-gpios = <&pio 2 14 GPIO_ACTIVE_HIGH>;     //PC14
    };
};
```
# 3 ways for it   
# 1. no parameter, it will use device tree, if width and height are both invalid in device tree, it will use default:240x240 
```shell
insmod drm-st7789.ko 
```
# 2. parameter, hd=240 vd=240 hd_mm=23 vd_mm=23
```shell
insmod drm-st7789.ko hd=240 vd=240 hd_mm=23 vd_mm=23
```
# 3. device tree
add propertys into yor spi device node, such as:
```
width = <240>;
height = <240>;
width-mm = <23>;
height-mm = <23>;
```