import java.io.*;
import java.util.Objects;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class Tema2 {
	public static void main(String[] args) throws FileNotFoundException {
		String folder = args[0];
		int thread_num = Integer.parseInt(args[1]);
		Thread[] threads = new Thread[thread_num];

		/* executor for the level 2 threads, so there can never be more than
		 thread_num threads running at the same time */
		ThreadPoolExecutor executor = new ThreadPoolExecutor(thread_num, thread_num,
				0L, TimeUnit.MILLISECONDS, new LinkedBlockingQueue<>());

		for (int i = 0; i < thread_num; i++) {
			Thread thread = new Thread(new ThreadLevel1(folder , i, executor));
			threads[i] = thread;
			thread.start();
		}

		for (int i = 0; i < thread_num; i++) {
			try {
				threads[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}


	static class ThreadLevel1 implements Runnable {
		private static int currentLine = 0;
		private final int id;
		private final String folderName;
		private static BufferedReader br = null;
		private static ThreadPoolExecutor executor = null;

		public ThreadLevel1(String folderName, int id, ThreadPoolExecutor executor) throws FileNotFoundException {
			this.id = id;
			this.folderName = folderName;
			if (br == null) {
				/* all threads use the same buffer reader to not be any overlap since buffer reader is synchronized */
				br = new BufferedReader(new FileReader(folderName + "/orders.txt"));
			}
			if (ThreadLevel1.executor == null) {
				ThreadLevel1.executor = executor;
			}
		}

		@Override
		public void run() {
			String  line = null;
			while (true) {
				try {
					if ((line = br.readLine()) == null) {
						executor.shutdown();
						break;
					}
				} catch (IOException  e) {
					e.printStackTrace();
				}

				synchronized (this) {
					currentLine++;
				}

				String[] info = line.split(",");

				ThreadLevel2 task_thread = new ThreadLevel2(folderName + "/order_products.txt");
				int nr_products = Integer.parseInt(info[1]);
				ShippedOrder shipper = new ShippedOrder(nr_products);

				for (int i = 1; i <= nr_products; i++) {
					int finalI = i;
					executor.execute(()->task_thread.Task(info[0], finalI, shipper));
				}


				try {
					// wait for order to be shipped
					executor.awaitTermination(1, TimeUnit.SECONDS);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}

				if (nr_products > 0) {
					appendToFile("orders_out.txt", line + ",shipped\n");
				}
			}
		}

		public synchronized void appendToFile(String fileName, String textToAppend) {
			File file = new File(fileName);
			if (!file.exists()) {
				try {
					file.createNewFile();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}

			try (FileWriter fileWriter = new FileWriter(fileName, true)) {
				fileWriter.write(textToAppend);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}

	}

	static class ThreadLevel2 {
		private  String fileName;

		public ThreadLevel2(String fileName) {
			this.fileName = fileName;
		}

		public void Task(String order,  int item_nr, ShippedOrder shipper) {
			BufferedReader br = null;
			try {
				br = new BufferedReader(new FileReader(fileName));
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			}

			String  line = null;
			int r = item_nr;
			while (true) {

				try {
					if ((line = br.readLine()) == null) break;
				} catch (IOException e) {
					e.printStackTrace();
				}

				//System.out.println("looking for [" + order + "][" + r+ "]: " + line + "\n\talready found: " + (r - item_nr));

				String[] info = line.split(",");
				if (Objects.equals(info[0], order)) {
					item_nr--;

					if (item_nr == 0){
						/* searched item found */
						//System.out.println("shipped item " + info[1]);
						synchronized (this) {
							shipper.shipped_nr++;
							appendToFile("order_products_out.txt", line + ",shipped\n");
						}
						break;
					}
				}
			}
		}

		public synchronized void appendToFile(String fileName, String textToAppend) {
			File file = new File(fileName);
			if (!file.exists()) {
				try {
					file.createNewFile();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}

			try (FileWriter fileWriter = new FileWriter(fileName, true)) {
				fileWriter.write(textToAppend);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	/* kind of a wrapper class to know if whole order has been shipped */
	static class ShippedOrder {

		private final int total_items;
		public int shipped_nr;

		public ShippedOrder(int to_be_shipped) {
			total_items = to_be_shipped;
		}

		public boolean Shipped() {
			return shipped_nr == total_items;
		}
	}
}
