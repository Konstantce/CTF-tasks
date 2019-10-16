__author__ = 'Konstantce'
import math
from fractions import Fraction


def continued_fraction(number):
    pp = qq = 1
    ap = math.trunc(number)
    number = 1 / (number - ap)
    a = math.trunc(number)
    p = ap * a + 1
    q = a
    while True:
        yield p, q
        number = 1 / (number - a)
        a = math.trunc(number)
        p, pp = a * p + pp, p
        q, qq = a * q + qq, q


def VinerAttack(e, N):
    TEST_NUMBER = 0x1212121211213L % N
    for _, d in continued_fraction(Fraction(e, N)):
        print d
        if pow(TEST_NUMBER, e * d, N) == TEST_NUMBER:
            print "YES!", d
            break
    return d


#VinerAttack(6792605526025, 9449868410449)


ciphertext = "3eebc162f287a5d465a78737e6794a9652eb2e4d31eaa96189a20a4e2a2c8c8cd3716fddd096d65944354d8888e0c79379687210eeec406b09ed872cfdcd4a6d37b04d7947d6c50435163b26d9d2457de1c812d62ee984ccbb98a644219958117cd9d1708a73641386da54c60acfded20fe1c45b8a23851ca17254b1126d5a54"
e = 0x694a8e65b58c5e95e9be62dfb3a09e42886fdb7d8f702a8940f049b9ceb63d76551cb046db4437770c6926b93da0548493c5b892dea4e6e2642a36ec9bd7a9b1da3b3de748974d63144df335d0e6a9fd6faf7646c63b02dda3e260aad16bb491c940275c865f5d5773efbc03f5eaa3f69c98b69838d6190719edb25378d3a2f7L
N = 0xa7ada5ae7985022534c7fe522000b3bdf9e80e879ccc3d8f334fef8cd96658e6715925e46a11fc221c1c28f4b411efd57272c95420f584b1e8b44c9f96e69539b545de4f2e66534dbe498e5427a4e79969361de97163686309f1c3f387e23609c60fd107fe656737825baa66577019dfcdf9cb1e1f49bb3b117e168d91598ec5L


#e = 0x207878518f95d460c9ce3cc575254c499d5a4fae41b64b80201d4101ce2d377a4d07aed0f4178e95ae78cc20a055f46d2dd1053df25916fd88c58aca84c12fcf0511106cc6c8a1d3be72bfabe7c020fc1c6a8a8acf54af3053fd61f299c0529c4b24043c3bd5f0de085ebcfd0ea87f122ec4fff3763aa9433cef27e1d7e16f3fL
#1117821600761499035054720718648407388218021023119778131752819457891003486403
#N=0x6d8854906ea3bcba8934762af105995e290f39e8c83fbc7927336d437d5e26d85a7762a5d8985d5a44d5f5f6c26c733685fa428b82b24847f15afb90da1fe08b86f29e33beb15fe62ca4f5350ab53a1abc265491ff31d136c4d345eef1e0c2b71326a15e7aa8a2aca765b924cc96c40241f9e6b03bd95a7561264c9f98c893c5L

d = VinerAttack(e, N)

result = hex(pow(int(ciphertext, 0x10), d, N))[2:].rstrip("lL")
import binascii
print binascii.unhexlify(result)
