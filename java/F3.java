public class F3 {
	public float x, y, z;

	public F3() {
		this(0, 0, 0);
	}

	public F3(float x, float y, float z) {
		this.x = x;
		this.y = y;
		this.z = z;
	}

	public float norm() {
		return (float) Math.sqrt(x * x + y * y + z * z);
	}

	public float dot(F3 other) {
		return x * other.x + y * other.y + z * other.z;
	}
}
