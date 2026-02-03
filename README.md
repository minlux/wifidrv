
## Build
```
gcc wifidrv.c 
```
## Run

```
./a.out 
```

This will produce a `wifidrv.img` to be mounted.


## Mount

```
sudo mount -o loop ./wifidrv.img ./mnt
```
