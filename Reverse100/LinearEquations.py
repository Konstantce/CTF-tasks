from numpy import array, linalg
import random, math

def xgcd(a, b):
    """
    Extented Euclid GCD algorithm.
    Return (x, y, g) : a * x + b * y = gcd(a, b) = g.
    """
    if a == 0:
        return 0, 1, b
    if b == 0:
        return 1, 0, a
    px, ppx = 0, 1
    py, ppy = 1, 0
    while b:
        q = a // b
        a, b = b, a % b
        x = ppx - q * px
        y = ppy - q * py
        ppx, px = px, x
        ppy, py = py, y
    return ppx, ppy, a


def gcd(a, b):
    return xgcd(a, b)[2]

flag = "linear_equations"
size = 0x10
modulus = size ** 2
vector = (map(ord, flag))
for i in xrange(size):
    vector[i] *= (i+1)
print vector







