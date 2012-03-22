public class F4 {
	public float x, y, z, w;

	public F4(float x, float y, float z, float w) {
		this.x = x;
		this.y = y;
		this.z = z;
		this.w = w;
	}

	public float norm() {
		return (float) Math.sqrt(x * x + y * y + z * z + w * w);
	}

	public float dot(F4 other) {
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}
}
