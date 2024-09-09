

// #define QUATERNION_TEMPLATE_quaternion_t vec2
// #define QUATERNION_TEMPLATE_scalar_t float
// #define QUATERNION_TEMPLATE_unsigned_scalar_t float
// #define QUATERNION_TEMPLATE_vec3 vec3
// #define QUATERNION_TEMPLATE_real_t float
// #define QUATERNION_TEMPLATE_CONTINUOUS 1
// #define QUATERNION_TEMPLATE_polar_quaternion_t polar_quaternion_t

QUATERNION_TEMPLATE_quaternion_t MakeQuaternion(QUATERNION_TEMPLATE_scalar_t a, QUATERNION_TEMPLATE_vec3 v)
{
	QUATERNION_TEMPLATE_quaternion_t res;
	res.x = a;
	res.yzw = v;
	return res;
}

QUATERNION_TEMPLATE_quaternion_t MakeQuaternion(QUATERNION_TEMPLATE_vec3 v)
{
	QUATERNION_TEMPLATE_quaternion_t res;
	res.x = 0;
	res.yzw = v;
	return res;
}

// Warning the returned Quaternion is not normalized
QUATERNION_TEMPLATE_quaternion_t MakeAxisAngleQuaternion(QUATERNION_TEMPLATE_vec3 axis, QUATERNION_TEMPLATE_real_t angle)
{
	QUATERNION_TEMPLATE_quaternion_t res;
	QUATERNION_TEMPLATE_real_t half_angle = angle / 2;
	res.x = cos(half_angle);
	res.yzw = axis * sin(half_angle);
	return res;
}

QUATERNION_TEMPLATE_scalar_t Real(QUATERNION_TEMPLATE_quaternion_t q)
{
	return q.x;
}

QUATERNION_TEMPLATE_vec3 Imaginary(QUATERNION_TEMPLATE_quaternion_t q)
{
	return q.yzw;
}

QUATERNION_TEMPLATE_scalar_t Modulus2(QUATERNION_TEMPLATE_quaternion_t q)
{
	return length2(q);
}

QUATERNION_TEMPLATE_real_t Modulus(QUATERNION_TEMPLATE_quaternion_t q)
{
	return length(q);
}

QUATERNION_TEMPLATE_quaternion_t Conjugate(QUATERNION_TEMPLATE_quaternion_t q)
{
	return QUATERNION_TEMPLATE_quaternion_t(Real(q), -Imaginary(q));
}


#if QUATERNION_TEMPLATE_CONTINUOUS

QUATERNION_TEMPLATE_quaternion_t Normalize(QUATERNION_TEMPLATE_quaternion_t q)
{
	return normalize(q);
}

#endif


QUATERNION_TEMPLATE_quaternion_t quaternion_add(QUATERNION_TEMPLATE_quaternion_t a, QUATERNION_TEMPLATE_quaternion_t b)
{
	return a + b;
}

QUATERNION_TEMPLATE_quaternion_t quaternion_sub(QUATERNION_TEMPLATE_quaternion_t a, QUATERNION_TEMPLATE_quaternion_t b)
{
	return a - b;
}

QUATERNION_TEMPLATE_quaternion_t quaternion_mul(QUATERNION_TEMPLATE_quaternion_t a, QUATERNION_TEMPLATE_quaternion_t b)
{
	QUATERNION_TEMPLATE_quaternion_t res;
	res.x = Real(a) * Real(b) - dot(Imaginary(a), Imaginary(b));
	res.yzw = Real(a) * Imaginary(b) + Real(b) * Imaginary(a) + cross(Imaginary(a), Imaginary(b));
	return res;
}

QUATERNION_TEMPLATE_quaternion_t quaternion_rcp(QUATERNION_TEMPLATE_quaternion_t q)
{
	return Conjugate(q) / Modulus2(q);
}

QUATERNION_TEMPLATE_quaternion_t quaternion_div(QUATERNION_TEMPLATE_quaternion_t a, QUATERNION_TEMPLATE_quaternion_t b)
{
	return quaternion_mul(a, quaternion_rcp(b));
}

// Rotate r by q
QUATERNION_TEMPLATE_quaternion_t rotate(QUATERNION_TEMPLATE_quaternion_t q, QUATERNION_TEMPLATE_quaternion_t r)
{
	QUATERNION_TEMPLATE_quaternion_t res = quaternion_mul(q, quaternion_mul(r, Conjugate(q)));
	return res;
}

QUATERNION_TEMPLATE_vec3 rotate(QUATERNION_TEMPLATE_quaternion_t q, QUATERNION_TEMPLATE_vec3 v)
{
	QUATERNION_TEMPLATE_quaternion_t res = rotate(q, MakeQuaternion(v));
	return Imaginary(res);
}

QUATERNION_TEMPLATE_real_t getQuaternionAngle(QUATERNION_TEMPLATE_quaternion_t q)
{
	return 2 * acos(Real(q));
}

QUATERNION_TEMPLATE_quaternion_t quaternion_sqr(QUATERNION_TEMPLATE_quaternion_t q)
{
	return quaternion_mul(q, q);
}




#if QUATERNION_TEMPLATE_CONTINUOUS

QUATERNION_TEMPLATE_quaternion_t quaternion_exp(QUATERNION_TEMPLATE_quaternion_t q)
{
	QUATERNION_TEMPLATE_quaternion_t res;
	// TODO test both impls
#if 0
	const QUATERNION_TEMPLATE_scalar_t m = Modulus(q);
	res = q * sinc(m);
	res.x += cos(m);
#else 
	const QUATERNION_TEMPLATE_scalar_t ea = exp(Real(q));
	const QUATERNION_TEMPLATE_scalar_t m = length(Imaginary(q));
	res.x = ea * cos(m);
	res.yxz = ea * Imaginary(q) * sinc(m);
#endif
	return res;
}

#endif