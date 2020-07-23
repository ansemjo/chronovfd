EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 2
Title ""
Date ""
Rev "1"
Comp ""
Comment1 "Designed for AISLER 2-Layer Service"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Sheet
S 2300 2150 1150 700 
U 5F189B7D
F0 "Universal VFD PSU by Rolo Kamp" 50
F1 "universal_vfd_psu.sch" 50
F2 "5V" I L 2300 2200 50 
F3 "HV" O R 3450 2350 50 
F4 "~HV_SHDN~" I L 2300 2450 50 
F5 "GND" I L 2300 2800 50 
F6 "FIL_SHDN" I L 2300 2550 50 
F7 "FIL_B" O R 3450 2650 50 
F8 "FIL_A" O R 3450 2550 50 
$EndSheet
$Comp
L IVL2-7_5:IVL2-5_7 U3
U 1 1 5F192B05
P 9600 1900
F 0 "U3" H 10481 1971 50  0000 L CNN
F 1 "IVL2-5_7" H 10481 1880 50  0000 L CNN
F 2 "extra_libraries:IVL2-7_5" H 8650 2150 50  0001 C CNN
F 3 "" H 8650 2150 50  0001 C CNN
	1    9600 1900
	1    0    0    -1  
$EndComp
Text GLabel 3500 2550 2    50   Input ~ 0
FILAMENT+
Text GLabel 3500 2650 2    50   Input ~ 0
FILAMENT-
Text GLabel 8850 1300 1    50   Input ~ 0
FILAMENT+
Text GLabel 8800 2500 3    50   Input ~ 0
FILAMENT+
Text GLabel 10350 1300 1    50   Input ~ 0
FILAMENT-
Text GLabel 10400 2500 3    50   Input ~ 0
FILAMENT-
Wire Wire Line
	3500 2550 3450 2550
Wire Wire Line
	3450 2650 3500 2650
$Comp
L HV5812:HV5812WG U4
U 1 1 5F19BBE0
P 7800 2600
F 0 "U4" H 7825 2665 50  0000 C CNN
F 1 "HV5812WG" H 7825 2574 50  0000 C CNN
F 2 "Package_SO:SOIC-28W_7.5x17.9mm_P1.27mm" H 7800 2600 50  0001 C CNN
F 3 "http://www.microchip.com/mymicrochip/filehandler.aspx?ddocname=en570713" H 7800 2600 50  0001 C CNN
	1    7800 2600
	1    0    0    -1  
$EndComp
$Comp
L power:+24V #PWR0105
U 1 1 5F1A10B2
P 3950 2250
F 0 "#PWR0105" H 3950 2100 50  0001 C CNN
F 1 "+24V" H 3965 2423 50  0000 C CNN
F 2 "" H 3950 2250 50  0001 C CNN
F 3 "" H 3950 2250 50  0001 C CNN
	1    3950 2250
	1    0    0    -1  
$EndComp
Wire Wire Line
	3450 2350 3950 2350
Wire Wire Line
	3950 2350 3950 2250
$Comp
L power:+24V #PWR0106
U 1 1 5F1A16B0
P 7200 2800
F 0 "#PWR0106" H 7200 2650 50  0001 C CNN
F 1 "+24V" H 7215 2973 50  0000 C CNN
F 2 "" H 7200 2800 50  0001 C CNN
F 3 "" H 7200 2800 50  0001 C CNN
	1    7200 2800
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0107
U 1 1 5F1A1AD8
P 6950 2800
F 0 "#PWR0107" H 6950 2650 50  0001 C CNN
F 1 "+5V" H 6965 2973 50  0000 C CNN
F 2 "" H 6950 2800 50  0001 C CNN
F 3 "" H 6950 2800 50  0001 C CNN
	1    6950 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	7200 2800 7200 2850
Wire Wire Line
	7200 2850 7300 2850
Wire Wire Line
	7300 2950 6950 2950
Wire Wire Line
	6950 2950 6950 2800
$Comp
L power:GND #PWR0108
U 1 1 5F1A2A0D
P 7200 4800
F 0 "#PWR0108" H 7200 4550 50  0001 C CNN
F 1 "GND" H 7205 4627 50  0000 C CNN
F 2 "" H 7200 4800 50  0001 C CNN
F 3 "" H 7200 4800 50  0001 C CNN
	1    7200 4800
	1    0    0    -1  
$EndComp
Wire Wire Line
	7300 4750 7200 4750
Wire Wire Line
	7200 4750 7200 4800
Entry Wire Line
	8650 4550 8750 4450
Entry Wire Line
	8650 4650 8750 4550
Entry Wire Line
	8650 4750 8750 4650
Entry Wire Line
	8650 4450 8750 4350
Entry Wire Line
	8650 4350 8750 4250
Entry Wire Line
	8650 4250 8750 4150
Entry Wire Line
	8650 4150 8750 4050
Entry Wire Line
	8650 4050 8750 3950
Entry Wire Line
	8650 3950 8750 3850
Entry Wire Line
	8650 3850 8750 3750
Entry Wire Line
	8650 3750 8750 3650
Entry Wire Line
	8650 3650 8750 3550
Entry Wire Line
	8850 3050 8950 2950
Entry Wire Line
	8950 3050 9050 2950
Entry Wire Line
	9050 3050 9150 2950
Entry Wire Line
	9150 3050 9250 2950
Entry Wire Line
	9250 3050 9350 2950
Entry Wire Line
	9350 3050 9450 2950
Entry Wire Line
	9450 3050 9550 2950
Entry Wire Line
	9550 3050 9650 2950
Entry Wire Line
	9650 3050 9750 2950
Entry Wire Line
	9750 3050 9850 2950
Entry Wire Line
	9850 3050 9950 2950
Entry Wire Line
	9950 3050 10050 2950
Entry Wire Line
	10050 3050 10150 2950
Entry Wire Line
	10150 3050 10250 2950
Entry Wire Line
	8650 3550 8750 3450
Entry Wire Line
	8650 3450 8750 3350
Wire Wire Line
	8950 2450 8950 2950
Wire Wire Line
	9050 2950 9050 2450
Wire Wire Line
	9150 2450 9150 2950
Wire Wire Line
	9250 2950 9250 2450
Wire Wire Line
	9350 2450 9350 2950
Wire Wire Line
	9450 2950 9450 2450
Wire Wire Line
	9550 2450 9550 2950
Wire Wire Line
	9650 2950 9650 2450
Wire Wire Line
	9750 2950 9750 2450
Wire Wire Line
	9850 2450 9850 2950
Wire Wire Line
	9950 2950 9950 2450
Wire Wire Line
	10050 2450 10050 2950
Wire Wire Line
	10150 2950 10150 2450
Wire Wire Line
	10250 2450 10250 2950
Wire Wire Line
	8650 3450 8350 3450
Wire Wire Line
	8350 3550 8650 3550
Wire Wire Line
	8650 3650 8350 3650
Wire Wire Line
	8350 3750 8650 3750
Wire Wire Line
	8650 3850 8350 3850
Wire Wire Line
	8350 3950 8650 3950
Wire Wire Line
	8650 4050 8350 4050
Wire Wire Line
	8350 4150 8650 4150
Wire Wire Line
	8650 4250 8350 4250
Wire Wire Line
	8350 4350 8650 4350
Wire Wire Line
	8650 4450 8350 4450
Wire Wire Line
	8350 4550 8650 4550
Wire Wire Line
	8350 4650 8650 4650
Wire Wire Line
	8650 4750 8350 4750
Wire Wire Line
	9050 1300 9050 1100
Wire Wire Line
	9350 1300 9350 1100
Wire Wire Line
	9550 1300 9550 1100
Wire Wire Line
	9850 1300 9850 1100
Wire Wire Line
	10150 1300 10150 1100
Wire Wire Line
	10350 2450 10400 2450
Wire Wire Line
	10400 2450 10400 2500
Wire Wire Line
	8850 2450 8800 2450
Wire Wire Line
	8800 2450 8800 2500
Wire Bus Line
	8750 3050 8750 4750
Wire Bus Line
	8750 3050 10200 3050
$EndSCHEMATC
