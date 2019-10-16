a="\x98\x9d\x84\x57\xba\x4f\x1c\x2f\x42\x19\xb5\xa3\x1f\xc1\x51\x17\xfc\x1e\x41\xf7\xf5\x31\x75\x1a\x14\x8c\x77\xf5\xbe\xcc\x1f\x2a"
b="\xca\xc8\xc7\x03\xfc\x10\x28\x1f\x7a\x7f\x8c\x94\x2e\xf9\x69\x24\x9f\x7d\x27\xc1\xc4\x09\x45\x7f\x75\xee\x45\x97\x8d\xaf\x79\x1f"

def func(a,b):

import math
import random
a=0x100
N=5

#-------------------------------------------------------------------------------------------
# Numeric integration does not give the result quickly

expression=(lambda(x):(math.sin(a*x/2)/math.sin(x/2))**(2*N))

def numerical_integration(func,N,a,b):
    S=0
    h=(b-a)/float(N)
    for index in xrange(N):
        u=a+index*h+h/2
        S+=expression(u)
    return S*h

#----------------------------------------------------------------------------------------
#Combinatirics - right way

import operator as op


def lucky_ticket_iter(a,N):
    #assumes that N=2*k, where k is odd
    #N - the number of positions it the ticket (6,...)
    #a - base for a number in the ticket (10,0x100)
    
    #binomial coefficient
    def ncr(n, r):
        r = min(r, n-r)
        if r == 0: return 1
        numer = reduce(op.mul, xrange(n, n-r, -1))
        denom = reduce(op.mul, xrange(1, r+1))
        return numer//denom
    
    s=(a-1)*N/2
    i=0
    sgn=1
    while (s>0):
        yield sgn*ncr(N,i)*ncr(s+N-1,N-1)
        sgn*=-1
        i+=1
        s-=a
    
def lucky_ticket(a,N):
    return reduce(lambda  x,y: x+y,lucky_ticket_iter(a,N),0) 

#--------------------------------------------------------------------------------
# Too slow......

import itertools
def proof(a,N):
    arr=itertools.product(range(a),repeat=N)
    sum_dict={}
    for ticket in arr:
        value=reduce(lambda x,y:x+y,ticket,0)
        if value in sum_dict:
            sum_dict[value]+=1
        else:
            sum_dict[value]=1
    sum=reduce(lambda x,y:x+y*y,sum_dict.values(),0)
    return sum

#-----------------------------------------------------------------------

a=0x100
N=5
    
print hex(lucky_ticket(a,2*N))


    
    
    
    
    
    
