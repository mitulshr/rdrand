import binascii
from rdrand import RdRandom
r =  RdRandom()
x16 = r.getrandbytes(2);
x32 = r.getrandbytes(4);
x64 = r.getrandbytes(8);
y16 = binascii.hexlify(x16)
y32 = binascii.hexlify(x32)
y64 = binascii.hexlify(x64)

print r.random();
	
print "16 Bits Random Hex: ", y16
print "16 Bits Number Dec: ", int(y16, 16), "\n"

print "32 Bits Random Hex: ", y32
print "32 Bits Number Dec: ", int(y32, 16), "\n"

print "64 Bits Random Hex: ", y64
print "64 Bits Number Dec: ", int(y64, 16)
