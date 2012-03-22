public class Clock {
	private final String msg;
	private final long start;
	private long last;

	public Clock() {
		StackTraceElement[] elems = Thread.currentThread().getStackTrace();
		StackTraceElement elem = elems[1];
		this.msg = elem.getFileName() + elem.getLineNumber() + " " + elem.getClassName() + "." + elem.getMethodName();
		start = last = System.currentTimeMillis();
	}

	public Clock(String msg) {
		this.msg = msg;
		start = last = System.currentTimeMillis();
	}

	/**
	 * Returns how many milliseconds elapsed since the clock was created.
	 */
	public long ms() {
		return (System.currentTimeMillis() - start);
	}

	public void print() {
		long now = System.currentTimeMillis();
		System.out.println(msg + ": " + (now - last) + "ms (total " + (now - start) + "ms)");
		last = System.currentTimeMillis();
	}

	public void print(String msg) {
		long now = System.currentTimeMillis();
		System.out.println(this.msg + " " + msg + ": " + (now - last) + "ms (total " + (now - start) + "ms)");
		last = System.currentTimeMillis();
	}
}
