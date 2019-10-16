flag="NotBadFor300Points!"
result=0
for i in flag:
	result*=307;
	result+=ord(i)
hardcoded=bin(result)[2:]
print hardcoded

found_flag=""
while(result):
	symbol=(result % 307)
	found_flag=chr(symbol)+found_flag
	result=(result - symbol) / 307
print found_flag,len(found_flag)
