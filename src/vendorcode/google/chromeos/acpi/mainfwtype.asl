
Scope(\) {
Name(ALTO, 0x00)    // AltOS = 1
Name(MFWT, 0x00)    // Main fw type, 4= legacy

Method(_INI,0, Serialized)
{
	Store(0x00, MFWT)
	Store(0x00, ALTO)
	Store(VBT7, MFWT)
	If(LEqual(MFWT, 0x04))
	{
		Store(0x01,ALTO)
	}

}
}
