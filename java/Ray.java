public class Ray {
	public F3 m, b;

	/**
	 * Represents a ray in 3D space. Formula: v = m*t + b, so b is the origin and m is the dir. t >= 0.
	 */
	public Ray(F3 m, F3 b) {
		this.m = m;
		this.b = b;
	}

}
