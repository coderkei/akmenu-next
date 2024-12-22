	.TEXT
	.ARM

@---------------------------------------------------------------------------------------
.GLOBAL     swiSwitchToGBAModeFixed
.func swiSwitchToGBAModeFixed
@---------------------------------------------------------------------------------------
swiSwitchToGBAModeFixed:
	mov r2,#0x40
	swi 0x1f0000

.endfunc

	.end

