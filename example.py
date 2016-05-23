import binascii
from rdrand import RdRandom
from rdseed import RdSeed

print "=================  Start RDRAND =====================\n"

r1 =  RdRandom()
x16 = r1.getrandbytes(2);
x32 = r1.getrandbytes(4);
x64 = r1.getrandbytes(8);
y16 = binascii.hexlify(x16)
y32 = binascii.hexlify(x32)
y64 = binascii.hexlify(x64)

print "Random Number:      ", r1.random(), "\n"
	
print "16 Bits Random Hex: ", y16
print "16 Bits Number Dec: ", int(y16, 16), "\n"

print "32 Bits Random Hex: ", y32
print "32 Bits Number Dec: ", int(y32, 16), "\n"

print "64 Bits Random Hex: ", y64
print "64 Bits Number Dec: ", int(y64, 16)


print "\n=================  End Of RDRAND =====================\n"
print "\n=================  Start RDSEED =====================\n"

r2 = RdSeed()
a16 = r2.getseedbytes(2);
a32 = r2.getseedbytes(4);
a64 = r2.getseedbytes(8);
b16 = binascii.hexlify(a16)
b32 = binascii.hexlify(a32)
b64 = binascii.hexlify(a64)

print "16 Bits Random Hex: ", b16
print "16 Bits Number Dec: ", int(b16, 16), "\n"

print "32 Bits Random Hex: ", b32
print "32 Bits Number Dec: ", int(b32, 16), "\n"

print "64 Bits Random Hex: ", b64
print "64 Bits Number Dec: ", int(b64, 16)

print "\n=================  End Of RDRAND =====================\n"
