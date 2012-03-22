public class Sphere {
	public F3 center, velocity;
	public float radius;

	public Sphere(F3 center, float radius) {
		this.center = center;
		this.radius = radius;
		this.velocity = new F3(0, 0, 0);
	}

	public float intersect(Ray ray, float max_t, F3 intersection, F3 normal) {
		/* compute ray-sphere intersection */
		F3 eye = ray.b;
		F3 dst = new F3(center.x - eye.x, center.y - eye.y, center.z - eye.z);
		float t = dst.dot(ray.m);
		/* is it in front of the eye? */
		if (t <= 0) {
			return -1;
		}
		/* depth test */
		float d = t * t - dst.dot(dst) + radius * radius;
		/* does it intersect the sphere? */
		if (d <= 0) {
			return -1;
		}
		/* is it closer than the closest thing we've hit so far */
		t -= (float) Math.sqrt(d);
		if (t >= max_t) {
			return -1;
		}

		/* if so, then we have an intersection */
		intersection.x = eye.x + t * ray.m.x;
		intersection.y = eye.y + t * ray.m.y;
		intersection.z = eye.z + t * ray.m.z;

		normal.x = (intersection.x - center.x) / radius;
		normal.y = (intersection.y - center.y) / radius;
		normal.z = (intersection.z - center.z) / radius;
		return t;
	}
}
