FILE := breadboard_psu_hbridge

$(FILE)-gerbers.zip: $(FILE).kicad_pcb
	kikit fab jlcpcb $< ./
	mv gerbers.zip $@
