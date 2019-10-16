import random
import binascii
import math


class Common_Math():
    def xgcd(self, a, b):
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

    def gcd(self, a, b):
        return self.xgcd(a, b)[2]


class PrimeNumberGenerator(Common_Math):
    small_primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163,
                    167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251]

    def prime_test(self):
        """
        Solovay-Strassen test
        """
        for _ in xrange(self.count):
            a = random.randint(2, self.number - 1)
            if self.gcd(a, self.number) != 1:
                return False
            result = pow(a, (self.number - 1) / 2, self.number)
            if (result - self.jacobi(a, self.number)) % self.number:
                return False
        return True

    def jacobi(self, a, b):
        """
        Return Jacobi symbol (or Legendre symbol if b is prime)
        """
        degree = 0
        while b > 1:
            if b & 1 == 0:
                raise ValueError("Jacobi is defined only for odd modules.")
            if a == 0:
                return 0
            elif a < 0:
                a = -a
                degree += (b-1)/2
            a = a % b
            counter = 0
            while not a % 2:
                counter += 1
                a /= 2
            if counter % 2:
                degree += (b*b-1)/8
            degree += (a-1)*(b-1)/4
            a, b = b, a
        if degree % 2:
            return -1
        else:
            return 1

    def initial_check(self):
        for elem in self.small_primes:
            if not (self.number % elem):
                return False
        return True

    def __init__(self, bit_length, repeat=100):
        self.bit_length = bit_length
        self.count = repeat

    def __call__(self):
        flag = False
        while not flag:
            self.number = random.getrandbits(self.bit_length)
            if not self.number % 2:
                self.number += 1
            if not (self.number / (2**(self.bit_length-1))):
                self.number += 2**(self.bit_length-1)
            flag = self.initial_check() and self.prime_test()
        return self.number


class RSA(Common_Math):
    MODE_ENCRYPT = 1
    MODE_DECRYPT = 0

    def __init__(self, primes_length):
        prime_generator = PrimeNumberGenerator(primes_length)
        p = prime_generator()
        q = prime_generator()
        euler_func = (p - 1) * (q - 1)
        self.modulus = p * q
        #Now generate public and private keys:
        while True:
            self.private = random.randint(3, math.pow(self.modulus, 0.25) / 3)
            a, b, g = self.xgcd(self.private, euler_func)
            if g != 1:
                continue
            else:
                self.public = a % euler_func
                break

    def encrypt(self, plaintext):
        return self.process(plaintext, self.MODE_ENCRYPT)

    def decrypt(self, ciphertext):
        return self.process(ciphertext, self.MODE_DECRYPT)

    def process(self, message, mode):
        number = int("0x" + message, 0x10)
        if number > self.modulus:
            raise ValueError("Incorrect message length.")
        if mode:
            degree = self.public
        else:
            degree = self.private
        number = pow(number, degree, self.modulus)
        data = hex(number)[2:].rstrip("Ll")
        return "0"*(len(data) % 2) + data

    def print_public_key(self):
        print "Public key: ", hex(self.public)
        print "Modulus: ", hex(self.modulus)


if __name__ == "__main__":
    flag = "*********************************************************"
    plaintext = binascii.hexlify(flag)
    rsa = RSA(512)
    print "Ciphertext:", rsa.encrypt(plaintext)
    rsa.print_public_key()

"""
OUTPUT:

Ciphertext: 3eebc162f287a5d465a78737e6794a9652eb2e4d31eaa96189a20a4e2a2c8c8cd3716fddd096d65944354d8888e0c79379687210eeec406b09ed872cfdcd4a6d37b04d7947d6c50435163b26d9d2457de1c812d62ee984ccbb98a644219958117cd9d1708a73641386da54c60acfded20fe1c45b8a23851ca17254b1126d5a54
Public key:  0x694a8e65b58c5e95e9be62dfb3a09e42886fdb7d8f702a8940f049b9ceb63d76551cb046db4437770c6926b93da0548493c5b892dea4e6e2642a36ec9bd7a9b1da3b3de748974d63144df335d0e6a9fd6faf7646c63b02dda3e260aad16bb491c940275c865f5d5773efbc03f5eaa3f69c98b69838d6190719edb25378d3a2f7L
Modulus:  0xa7ada5ae7985022534c7fe522000b3bdf9e80e879ccc3d8f334fef8cd96658e6715925e46a11fc221c1c28f4b411efd57272c95420f584b1e8b44c9f96e69539b545de4f2e66534dbe498e5427a4e79969361de97163686309f1c3f387e23609c60fd107fe656737825baa66577019dfcdf9cb1e1f49bb3b117e168d91598ec5L

"""







