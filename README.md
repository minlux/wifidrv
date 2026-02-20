# WiFi-Drive


## Hardware
https://docs.platformio.org/en/latest/boards/espressif32/esp32s3usbotg.html


## Software

### VS-Code + PlatformIO

In VS-Code wurde extra ein Profil für PlatformIO angelegt, damit es nur dann geladen wird, wenn man es auch braucht ...
VS-Code kann direkt mit diesem Profil gestartet werden:

```
code --profile PlatformIO .
```

### Mass Storage Device

Wir wollen einen "Storage" emulieren.
Um rauszufinden was wir alles brauchen, bauen wir den Storage erstmal mit einem normalen USB Stick
und analysieren diesen dann...


1. Dateien vorbereiten

Wir nehmen 2 Beispielbilder und und erweitern diese auf 128kB mit dem `truncate` Kommando.
Genause erstellen wir mit `truncate` eine leer 2kB Datei:

```
truncate -s 2k CREDS.JSN
truncate -s 128k IMG1.JPG
truncate -s 128k IMG2.JPG
```

2. Stick Partitionieren

Jetzt nehmen wir einen USB Stick und erstellem darauf, mit `gparted` eine 16MB Partition.

3. Formatieren

Das Formatieren der Partition kann man auch gleich im Schritt 2, in `gparted` machen,
oder aber wie folgt:

```
mkfs.fat -F 16 /dev/sdb1
```

4. Partition mounten

Jetzt die Partition des Sticks mounten:

```
mkdir -p /tmp/stick
mount /dev/sdb1 /tmp/stick
```

5. Daten kopieren

Nun die im Schritt 1 erstellten Dateien drauf kopieren:

```
cp CREDS.JSN /tmp/stick
cp IMG1.JPG /tmp/stick
cp IMG2.JPG /tmp/stick
```

6. Partition umounten

```
umount /tmp/stick
```

7. DD vom Stick

Jetzt holen wir uns ein 20MB Image vom Stick:

```
dd if=/dev/sdb of=stick16.img bs=4M count=5
```

8. Hexdump davon zum Anschauen

Wir konvertieren den Inhalt in Hexdump:

```
hd stick16.img > stick16.hex
```

9. Inhalt analysieren

Wir analysieren den Hexdump um die Startadressen von *MBR*, *VBR*, *FAT* und *ROOT-Directory* rauszufinden.
Wir nutzen nun die "Tools" (im Ordner `tools`), um den Inhalt zu dekodieren:

```
./read_mbr stick16.img 0 > layout.txt
./read_vbr stick16.img 0x00100000 >> layout.txt
./read_fat stick16.img 0x00100800 >> layout.txt
./read_rootdir stick16.img 0x00108800 >> layout.txt
```

10. Inhalt extrahieren

Wir extrahieren *MBR*, *VBR*, *FAT* und *ROOT-Directory* aus dem Image
in "C-Tabellen":

```
xxd -c 16 -s 0 -l 512 -i stick16.img > mbr.c
xxd -c 16 -s 1048576 -l 512 -i stick16.img > vbr.c
xxd -c 16 -s 1050624 -l 512 -i stick16.img > fat.c
xxd -c 16 -s 1083392 -l 512 -i stick16.img > rootdir.c
```

Dann die Tabellen und den Integer in den *C-Dateien* `const` machen und in den `src` Ordner kopieren.

