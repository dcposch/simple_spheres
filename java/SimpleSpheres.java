import java.awt.Graphics;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.awt.image.WritableRaster;
import java.util.ArrayList;
import java.util.List;

/**
 * Simple ray tracer test.
 */
public class SimpleSpheres extends java.awt.Frame {

	public static int GRID_SIZE = 2;
	public static float RADIUS = 0.2f;
	public static int MAX_TRACE_DEPTH = 3;
	public static int ANTIALIASING = 1;

	// render state
	private BufferedImage bufferA, bufferB;
	private int[] rgbs;
	private float t;
	private int width, height;

	// scene
	private final List<Sphere> spheres = new ArrayList<Sphere>();
	private final List<Light> lights = new ArrayList<Light>();
	private Vol3 bounds;
	private F3 eyeLocation;

	public SimpleSpheres() {
		super("Simple Spheres");
		setSize(800, 600);
		addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent e) {
				System.exit(0);
			}
		});
		System.setProperty("sun.awt.noerasebackground", "true");
		System.setProperty("sun.awt.erasebackgroundonresize", "false");

		initScene();
		initLights();

		new Thread(new Runnable() {
			@Override
			public void run() {
				renderLoop();
			}
		}).start();
	}

	private void initScene() {
		bounds = new Vol3(new F3(-1.5f, -1.5f, -1f), new F3(1.5f, 1.5f, 1f));
		for (int i = 0; i < GRID_SIZE; i++) {
			for (int j = 0; j < GRID_SIZE; j++) {
				for (int k = 0; k < GRID_SIZE; k++) {
					F3 center = new F3(i - 0.5f * (GRID_SIZE - 1), j - 0.5f * (GRID_SIZE - 1), k - 0.5f * (GRID_SIZE - 1));
					Sphere sphere = new Sphere(center, RADIUS);
					sphere.velocity = new F3((float) Math.random() / 30f, (float) Math.random() / 30f, (float) Math.random() / 30f);
					spheres.add(sphere);
				}
			}
		}
	}

	private void initLights() {
		lights.add(new Light());
		lights.add(new Light());
		lights.get(0).color = new F3(1, 1, 1);
		lights.get(1).color = new F3(0, 0, 1);
	}

	private void sleep(long millis) {
		try {
			Thread.sleep(millis);
		} catch (InterruptedException e) {

		}
	}

	private void renderLoop() {
		while (true) {
			if (getWidth() == 0 || getHeight() == 0) {
				sleep(100);
				continue;
			}

			Clock clk = new Clock();
			this.repaint();

			// create buffer, if needed
			width = getWidth();
			height = getHeight();
			if (bufferA == null) {
				bufferA = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
			} else if (bufferA.getWidth() != width || bufferA.getHeight() != height) {
				bufferA.flush();
				bufferA = null;
				System.gc();
				bufferA = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
			}

			// draw to buffer A
			timestep();
			traceImage(bufferA);

			// swap buffers
			BufferedImage tmp = bufferA;
			bufferA = bufferB;
			bufferB = tmp;

			// buffer B is the one that always gets rendered.
			System.out.println(clk.ms() == 0 ? "-" : new Long(1000 / clk.ms()).toString() + " FPS");
			t += clk.ms() / 1000f;
		}
	}

	private float absf(float f) {
		return f < 0 ? -f : f;
	}

	private float sinf(float f) {
		return (float) Math.sin(f);
	}

	private float cosf(float f) {
		return (float) Math.cos(f);
	}

	private void timestep() {

		/* eye, moves with time */
		eyeLocation = new F3(0, // sinf(t),
				0, -4f); // - cosf(t)};

		/* so does the light */
		for (int i = 0; i < lights.size(); i++) {
			Light light = lights.get(i);
			light.location = new F4(sinf(t / 2 + i) * cosf(t / 4 + i), sinf(t / 4 + i), cosf(t / 2 + i) * cosf(t / 4 + i), 0.2f);
		}

		/* so do the objs */
		for (int i = 0; i < spheres.size(); i++) {
			Sphere s = spheres.get(i);
			s.center.x += s.velocity.x;
			s.center.y += s.velocity.y;
			s.center.z += s.velocity.z;

			/* bounce off the bounding box */
			if (s.center.x < bounds.top.x) {
				s.velocity.x = absf(s.velocity.x);
			}
			if (s.center.y < bounds.top.y) {
				s.velocity.y = absf(s.velocity.y);
			}
			if (s.center.z < bounds.top.z) {
				s.velocity.z = absf(s.velocity.z);
			}
			if (s.center.x > bounds.bottom.x) {
				s.velocity.x = -absf(s.velocity.x);
			}
			if (s.center.y > bounds.bottom.y) {
				s.velocity.y = -absf(s.velocity.y);
			}
			if (s.center.z > bounds.bottom.z) {
				s.velocity.z = -absf(s.velocity.z);
			}

			/* bounce off each other */
			for (int j = i + 1; j < spheres.size(); j++) {
				Sphere s2 = spheres.get(j);
				F3 diff = new F3(s.center.x - s2.center.x, s.center.y - s2.center.y, s.center.z - s2.center.z);
				float dist = diff.norm();
				if (dist < s.radius + s2.radius) {
					diff.x /= dist;
					diff.y /= dist;
					diff.z /= dist;
					float dot = diff.dot(s.velocity);
					if (dot > 0) {
						/* if they're already flying apart,
						 * don't bounce them back towards each other */
						continue;
					}
					float dot2 = diff.dot(s2.velocity);
					s.velocity.x -= diff.x * (dot - dot2);
					s.velocity.y -= diff.y * (dot - dot2);
					s.velocity.z -= diff.z * (dot - dot2);
					s2.velocity.x -= diff.x * (dot2 - dot);
					s2.velocity.y -= diff.y * (dot2 - dot);
					s2.velocity.z -= diff.z * (dot2 - dot);
				}
			}
		}
	}

	private void traceImage(BufferedImage dest) {
		WritableRaster raster = dest.getRaster();
		DataBufferInt dataBuffer = (DataBufferInt) raster.getDataBuffer();
		rgbs = dataBuffer.getData();

		Clock clk = new Clock("traceImage");
		int width = dest.getWidth();
		int height = dest.getHeight();
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				F3 color = renderPixel(i, j);
				int rgb = (int) (color.x * 255.9f);
				rgb |= (int) (color.y * 255.9f) << 8;
				rgb |= (int) (color.z * 255.9f) << 16;
				rgbs[j * width + i] = rgb;
			}
		}
		clk.print();
	}

	private F3 expose(F3 incident, float e) {
		return new F3((float) (1.0 - Math.exp(-e * incident.x)), (float) (1.0 - Math.exp(-e * incident.y)), (float) (1.0 - Math.exp(-e
				* incident.z)));
	}

	private float intersect(Ray ray, F3 intersection, F3 normal) {
		/* main raytrace loop: go through geometry, testing for intersections */
		float tmin = 1000000;
		for (Sphere s : spheres) {
			/* get geometry */
			float t = s.intersect(ray, tmin, intersection, normal);
			if (t > 0 && t < tmin) {
				tmin = t;
			}
		}
		return (tmin < 100000) ? tmin : -1;
	}

	private float trace(Ray ray, F3 color, int max_depth) {
		color.x = color.y = color.z = 0;
		F3 intersection = new F3(), normal = new F3();
		float t = intersect(ray, intersection, normal);
		if (t < 0) {
			return -1;
		}

		/* shade; no shadows for now */
		F3 specular = new F3(0.4f, 0.4f, 0.4f);
		F3 diffuse = new F3(1, 1, 0.2f);
		for (Light light : lights) {
			F4 light_location = light.location;
			F3 light_direction = new F3(light_location.x - intersection.x * light_location.w, light_location.y - intersection.y
					* light_location.w, light_location.z - intersection.z * light_location.w);
			/* shadows -- these are slow.
			struct ray light_ray;
			light_ray.start = intersection;
			light_ray.direction = light_direction;
			f3 tmp_intersect, tmp_normal;
			if(intersect(&light_ray, state, &tmp_intersect, &tmp_normal) > 0)
			    continue;*/
			float cos_incident = normal.dot(light_direction) / light_direction.norm();
			if (cos_incident < 0) {
				cos_incident = 0;
			}
			cos_incident += 0.04f;
			color.x += diffuse.x * cos_incident * light.color.x;
			color.y += diffuse.y * cos_incident * light.color.y;
			color.z += diffuse.z * cos_incident * light.color.z;
		}

		/* recurse to get specular reflection */
		float bounce = ray.m.dot(normal);
		Ray newRay = new Ray(new F3(ray.m.x - 2 * bounce * normal.x, ray.m.y - 2 * bounce * normal.y, ray.m.z - 2 * bounce * normal.z),
				intersection);
		F3 newLight = new F3();
		if (max_depth > 0) {
			trace(newRay, newLight, max_depth - 1);
		}

		/* special case: specular highlights */
		for (Light light : lights) {
			F4 light_location = light.location;
			F3 light_direction = new F3(light_location.x - intersection.x * light_location.w, light_location.y - intersection.y
					* light_location.w, light_location.z - intersection.z * light_location.w);
			float light_cos = light_direction.dot(newRay.m);
			float thresh = 0.85f;
			if (light_cos > thresh) {
				float highlight = (light_cos - thresh) / (1 - thresh);
				highlight *= highlight * highlight * 3;
				newLight.x += highlight * light.color.x;
				newLight.y += highlight * light.color.y;
				newLight.z += highlight * light.color.z;
			}
		}
		color.x += specular.x * newLight.x;
		color.y += specular.y * newLight.y;
		color.z += specular.z * newLight.z;

		return t;
	}

	F3 renderPixel(int x, int y) {
		/* antialiasing */
		F3 avg_light = new F3();
		for (int i = 0; i < ANTIALIASING; i++) {
			F3 normalized = new F3(((x + (float) (i % 2) / 4) / width) * 2 - 1, 1 - ((y + (float) (i / 2) / 4) / height) * 2, 0);
			Ray r = new Ray(new F3(normalized.x / 2, normalized.y / 2, (float) Math.sqrt(1.0 - (normalized.x * normalized.x + normalized.y
					* normalized.y) / 4)), eyeLocation);
			F3 incident_light = new F3();
			trace(r, incident_light, MAX_TRACE_DEPTH);
			avg_light.x += incident_light.x / ANTIALIASING;
			avg_light.y += incident_light.y / ANTIALIASING;
			avg_light.z += incident_light.z / ANTIALIASING;
		}
		F3 color = expose(avg_light, 1);
		return color;
	}

	@Override
	public void update(Graphics g) {
		paint(g);
	}

	/**
	 * Performs double-buffering to prevent flicker. Don't use this, use paintBuffer().
	 */
	@Override
	public synchronized void paint(Graphics g) {
		if (bufferB != null) {
			g.drawImage(bufferB, 0, 0, this);
		}
	}

	public static void main(String[] args) {
		new SimpleSpheres().setVisible(true);
	}
}
