#---------------------------------------------------------------------------------------------------------------------------------
#Final sploit: consists of two parts - Dual_EC_DRBG attack on ECC and broadcast attack on RSA

#---------------------------------------------------------------------------------------------------------------------------------
#imports

import libnum
import os
import sympy

#------------------------------------------------------------------------------------------------------------------------------------
#Constants: paths for three files - cryptor.exe, strange.exe and output.exe
#Feel free to modify these values

cryptofile="C:\Users\Konstantce\Desktop\VolgaCTF\Crypto500\\for_players\strange.exe"
exefile="C:\Users\Konstantce\Desktop\VolgaCTF\Crypto500\\for_players\cryptor.exe"
outputfile="C:\Users\Konstantce\Desktop\VolgaCTF\Crypto500\\for_players\output.exe"

#-----------------------------------------------------------------------------------------------------------------------------------
#Dual_EC_DRBG backdoor: a proof of concept

#------------------------------------------------------------------------------------------------------------------------------------
#Elliptic Curve parameteres: y^2=x^3+a*x+b (mod p)
#F = (backdoor) * G - points on a curve

p=115792089210356248762697446949407573530086143415290314195533631308867097853951
order=115792089210356248762697446949407573529996955224135760342422259061068512044369
a=115792089210356248762697446949407573530086143415290314195533631308867097853948
b=41058363725152142129326129780047268409114441015993725554835256314039467401291
G = (48439561293906451759052585252797914202762949526041747995844080717082404635286,
     36134250956749795798585127919587881956611106672985015071877198253568414405109)

backdoor=238541930543


#-----------------------------------------------------------------------------------------------------------------------------------
#implementation

def get_str(a):
    result=hex(a)[2:]
    return "0"*(2-len(result))+result

def get_gamma_symbol(a,b):
    result=ord(a) ^ ord(b)
    result=hex(result)[2:]
    return "0"*(2-len(result))+result

def normalize(a):
    output=hex(a)[2:].rstrip("L")
    output=("0"*(64-len(output)))+output
    return output
    

def get_gamma(s):
    result=[]
    for i in xrange(32):
        result.append(int("0x"+s[2*i:2*i+2],0x10))
    return result

#---------------------------------------------------------------------------------------------------------------------------------------
#decryption function - one round of decryption cycle

def decryption(curve,seed,inputfile,outputfile,last=False):
    global F
    coordinate=curve.power(F,seed)[0]
    seed=curve.generate(seed)[0]
    gamma=get_gamma(normalize(coordinate))
    if not last:
        data=inputfile.read(32)
        for i in range(len(data)):
            outputfile.write(chr(gamma[i]^ord(data[i])))
    else:
        data=inputfile.read()
        for i in range(len(data)):
            outputfile.write(chr(gamma[i]^ord(data[i])))
    return seed
            
#-----------------------------------------------------------------------------------------------------------------------------------------
# Main function of this block    
    
def mainECC():
    global exefile
    exe=open(exefile,"rb")
    plaintext=exe.read(32)
    global cryptofile
    f=open(cryptofile,"rb")
    ciphertext=f.read(32)
    
    gamma=map(get_gamma_symbol,ciphertext,plaintext)
    coordinate=int("0x"+"".join(gamma),0x10)
    exe.close()
    
    #init elliptic Curve
    
    global a,b,p,order,G
    curve=libnum.ecc.Curve(a,b,p,order=order,g=G)
    points=curve.check_x(coordinate)
    if not points:
        raise Exception("No points on a curve with such x-coordinate!")
    global F
    d=libnum.invmod(backdoor,order)
    F=curve.generate(backdoor)
    valid_seeds=[]

    #here the key moment
    
    for point in points:
        value=curve.power(point,d)[0]
        valid_seeds.append(value)
    print valid_seeds
        
    #here we must choose one of the seeds - 0 or 1
    #feel free to modify this value
    
    seed=valid_seeds[1]
    global outputfile
    exe=open(outputfile,"wb")
    exe.write(plaintext)
    f.seek(32, 0)
    filesize=os.path.getsize(cryptofile)
    print filesize / 32
    #decryption_cycle:
    
    for _ in xrange(filesize / 32):
        print ("%d cycle" %_)
        seed=decryption(curve,seed,f,exe)    
    if filesize % 32:
        decryption(curve,seed,f,exe,True)
    f.close()
    exe.close()

    
#Hastad's Broadcast Attack on RSA
#-----------------------------------------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------------------------------------
# Algorithm realization

def str_to_num(s):
    return int(s.encode("HEX"),0x10)

def num_to_str(n):
    a=hex(n)[2:].rstrip("L")
    if  (len(a) % 2):
        a="0"+a
    return a.decode("HEX")


def RSA(plaintext,public_key):
    a=str_to_num(plaintext)
    e,N=public_key
    b=pow(a,e,N)
    return num_to_str(b)

def Broadcast_Attack(ciphertexts,public_exponent,modules):
    N=reduce(lambda res, x: res*x, modules, 1)
    result=0
    for i in xrange(len(modules)):
        temp=list(modules[:])
        value=temp.pop(i)
        module=reduce(lambda res,x:res*x,temp,1)
        opposite=(libnum.invmod(module,value))
        result+=str_to_num(ciphertexts[i])*(N / modules[i]) * opposite
    result = (result % N)
    result=result ** sympy.Rational(1,public_exponent)
    return num_to_str(int(result))

# ---------------------------------------------------------------------------------------------------------------------------------------------
#Global constants: modules, public_exponent e=3
    
N1=8769382569369657803086243072934885816580191253501078889741031803240287823072830795825348656470510124293608077663922619340686640086376656215519245633544865088584897320559789873866699364415357805509308987319042174775873847912600169477261907721200446628992152708829599994554654568048633400503489105287545371814842144683556457456632958355841259920030385121623436375290823187943439982609366341320431941859698682187290978401499850417885737393353382420046780581989144735256053452338371365283525729518137862098754137705638286990510374828572469405835006397291843317479629751232360581770781260313617159523339829310542865360973
N2=16641232558008082569072755981139929534310277003825636370117280360586792006173000345248870909405490163782428965357585774135952656969994563662987557444687981734473925434494610603841799408783771395787921030694228929449499603107463078533299168134221519095050604356235671000307081776968001942046185624486992839010649980386408074499989184671809470571404744580685508238120539156543844749096699573023156371314661322710816376349130587867338126777467459699166329585414171830665270870208609324522210567359784452559076441172625777002185091610881224062051166361789674985037719748547252902252135938183711795370174487562423790707821
N3=14838493139945955887283262879803721399112945940019282151226200084081889272810582747314798986067549731912126091191959539634485712464017530737841463675802189682390544800786692910048607162795234692100428547461834541302493669259032930743367853937448648874889495656623082778699124284153758531887057303280042685837060081844296748762758365988562601515791442455174424418175141460198432527291057117854054756662539680092253321643600725563101888819435496736721366683826469620939274209340835390946392215404593754680601775875311263660997430300461775357032651879275620247801041675654599200662668290657734330742506352599309225057563
    

#--------------------------------------------------------------------------------------------------------------------------------------------
# Main function of this block

def mainRSA():
    global exefile
    f=open(exefile+":secret","rb")
    data=f.read()
    print data
    f.close()
    global N1,N2,N3
    print Broadcast_Attack((data[:256],data[256:512],data[512:]),3,(N1,N2,N3))
    
#-----------------------------------------------------------------------------------------------------------------------------------------------
# Combining together

if __name__=="__main__":
    mainECC()
    mainRSA()
    
#------------------------------------------------------------------------------------------------------------------------------------------------