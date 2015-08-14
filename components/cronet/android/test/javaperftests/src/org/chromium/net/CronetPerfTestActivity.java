// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Debug;

import org.chromium.net.urlconnection.CronetHttpURLStreamHandler;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Runs networking benchmarks and saves results to a file.
 */
public class CronetPerfTestActivity extends Activity {
    // Benchmark configuration passed down from host via Intent data.
    // Call getConfig*(key) to extract individual configuration values.
    private Uri mConfig;
    // Functions that retrieve individual benchmark configuration values.
    private String getConfigString(String key) {
        return mConfig.getQueryParameter(key);
    }
    private int getConfigInt(String key) {
        return Integer.parseInt(mConfig.getQueryParameter(key));
    }
    private boolean getConfigBoolean(String key) {
        return Boolean.parseBoolean(mConfig.getQueryParameter(key));
    }

    private enum Mode {
        SYSTEM_HUC, // Benchmark system HttpURLConnection
        CRONET_HUC, // Benchmark Cronet's HttpURLConnection
        CRONET_ASYNC, // Benchmark Cronet's asynchronous API
    }
    private enum Direction {
        UP, // Benchmark upload (i.e. POST)
        DOWN, // Benchmark download (i.e. GET)
    }
    private enum Size {
        LARGE, // Large benchmark
        SMALL, // Small benchmark
    }
    private enum Protocol {
        HTTP,
        QUIC,
    }

    // Put together a benchmark configuration into a benchmark name.
    // Make it fixed length for more readable tables.
    // Benchmark names are written to the JSON output file and slurped up by Telemetry on the host.
    private static String buildBenchmarkName(
            Mode mode, Direction direction, Protocol protocol, int concurrency, int iterations) {
        String name = direction == Direction.UP ? "Up___" : "Down_";
        switch (protocol) {
            case HTTP:
                name += "H_";
                break;
            case QUIC:
                name += "Q_";
                break;
            default:
                throw new IllegalArgumentException("Unknown protocol: " + protocol);
        }
        name += iterations + "_" + concurrency + "_";
        switch (mode) {
            case SYSTEM_HUC:
                name += "SystemHUC__";
                break;
            case CRONET_HUC:
                name += "CronetHUC__";
                break;
            case CRONET_ASYNC:
                name += "CronetAsync";
                break;
            default:
                throw new IllegalArgumentException("Unknown mode: " + mode);
        }
        return name;
    }

    // Responsible for running one particular benchmark and timing it.
    private class Benchmark {
        private final Mode mMode;
        private final Direction mDirection;
        private final Protocol mProtocol;
        private final URL mUrl;
        private final String mName;
        private final UrlRequestContext mCronetContext;
        // Size in bytes of content being uploaded or downloaded.
        private final int mLength;
        // How many requests to execute.
        private final int mIterations;
        // How many requests to execute in parallel at any one time.
        private final int mConcurrency;
        // Dictionary of benchmark names mapped to times to complete the benchmarks.
        private final JSONObject mResults;
        // How large a buffer to use for passing content, in bytes.
        private final int mBufferSize;
        // Cached copy of getConfigBoolean("CRONET_ASYNC_USE_NETWORK_THREAD") for faster access.
        private final boolean mUseNetworkThread;

        private long mStartTimeMs = -1;
        private long mStopTimeMs = -1;

        /**
         * Create a new benchmark to run.  Sets up various configuration settings.
         * @param mode The API to benchmark.
         * @param direction The transfer direction to benchmark (i.e. upload or download).
         * @param size The size of the transfers to benchmark (i.e. large or small).
         * @param protocol The transfer protocol to benchmark (i.e. HTTP or QUIC).
         * @param concurrency The number of transfers to perform concurrently.
         * @param results Mapping of benchmark names to time required to run the benchmark in ms.
         *                When the benchmark completes this is updated with the result.
         */
        public Benchmark(Mode mode, Direction direction, Size size, Protocol protocol,
                int concurrency, JSONObject results) {
            mMode = mode;
            mDirection = direction;
            mProtocol = protocol;
            final String resource;
            switch (size) {
                case SMALL:
                    resource = getConfigString("SMALL_RESOURCE");
                    mIterations = getConfigInt("SMALL_ITERATIONS");
                    mLength = getConfigInt("SMALL_RESOURCE_SIZE");
                    break;
                case LARGE:
                    // When measuring a large upload, only download a small amount so download time
                    // isn't significant.
                    resource = getConfigString(
                            direction == Direction.UP ? "SMALL_RESOURCE" : "LARGE_RESOURCE");
                    mIterations = getConfigInt("LARGE_ITERATIONS");
                    mLength = getConfigInt("LARGE_RESOURCE_SIZE");
                    break;
                default:
                    throw new IllegalArgumentException("Unknown size: " + size);
            }
            final int port;
            switch (protocol) {
                case HTTP:
                    port = getConfigInt("HTTP_PORT");
                    break;
                case QUIC:
                    port = getConfigInt("QUIC_PORT");
                    break;
                default:
                    throw new IllegalArgumentException("Unknown protocol: " + protocol);
            }
            try {
                mUrl = new URL("http", getConfigString("HOST"), port, resource);
            } catch (MalformedURLException e) {
                throw new IllegalArgumentException("Bad URL: " + getConfigString("HOST") + ":"
                        + port + "/" + resource);
            }
            final UrlRequestContextConfig cronetConfig = new UrlRequestContextConfig();
            if (mProtocol == Protocol.QUIC) {
                cronetConfig.enableQUIC(true);
                cronetConfig.addQuicHint(getConfigString("HOST"), getConfigInt("QUIC_PORT"),
                        getConfigInt("QUIC_PORT"));
            }
            mCronetContext = new CronetUrlRequestContext(CronetPerfTestActivity.this, cronetConfig);
            mName = buildBenchmarkName(mode, direction, protocol, concurrency, mIterations);
            mConcurrency = concurrency;
            mResults = results;
            mBufferSize = mLength > getConfigInt("MAX_BUFFER_SIZE")
                    ? getConfigInt("MAX_BUFFER_SIZE")
                    : mLength;
            mUseNetworkThread = getConfigBoolean("CRONET_ASYNC_USE_NETWORK_THREAD");
        }

        private void startTimer() {
            mStartTimeMs = System.currentTimeMillis();
        }

        private void stopTimer() {
            mStopTimeMs = System.currentTimeMillis();
        }

        private void reportResult() {
            if (mStartTimeMs == -1 || mStopTimeMs == -1)
                throw new IllegalStateException("startTimer() or stopTimer() not called");
            try {
                mResults.put(mName, mStopTimeMs - mStartTimeMs);
            } catch (JSONException e) {
                System.out.println("Failed to write JSON result for " + mName);
            }
        }

        // TODO(pauljensen): Remove @SuppressLint once crbug.com/501591 is fixed.
        @SuppressLint("NewApi")
        private void startLogging() {
            if (getConfigBoolean("CAPTURE_NETLOG")) {
                mCronetContext.startNetLogToFile(getFilesDir().getPath() + "/" + mName + ".json",
                        false);
            }
            if (getConfigBoolean("CAPTURE_TRACE")) {
                Debug.startMethodTracing(getFilesDir().getPath() + "/" + mName + ".trace");
            } else if (getConfigBoolean("CAPTURE_SAMPLED_TRACE")) {
                Debug.startMethodTracingSampling(
                        getFilesDir().getPath() + "/" + mName + ".trace", 8000000, 10);
            }
        }

        private void stopLogging() {
            if (getConfigBoolean("CAPTURE_NETLOG")) {
                mCronetContext.stopNetLog();
            }
            if (getConfigBoolean("CAPTURE_TRACE") || getConfigBoolean("CAPTURE_SAMPLED_TRACE")) {
                Debug.stopMethodTracing();
            }
        }

        /**
         * Transfer {@code mLength} bytes through HttpURLConnection in {@code mDirection} direction.
         * @param connection The HttpURLConnection to use for transfer.
         * @param buffer A buffer of length |mBufferSize| to use for transfer.
         * @return {@code true} if transfer completed successfully.
         */
        private boolean exerciseHttpURLConnection(URLConnection urlConnection, byte[] buffer)
                throws IOException {
            final HttpURLConnection connection = (HttpURLConnection) urlConnection;
            try {
                int bytesTransfered = 0;
                if (mDirection == Direction.DOWN) {
                    final InputStream inputStream = connection.getInputStream();
                    while (true) {
                        final int bytesRead = inputStream.read(buffer, 0, mBufferSize);
                        if (bytesRead == -1) {
                            break;
                        } else {
                            bytesTransfered += bytesRead;
                        }
                    }
                } else {
                    connection.setDoOutput(true);
                    connection.setRequestMethod("POST");
                    connection.setRequestProperty("Content-Length", Integer.toString(mLength));
                    final OutputStream outputStream = connection.getOutputStream();
                    for (int remaining = mLength; remaining > 0; remaining -= mBufferSize) {
                        outputStream.write(buffer, 0, Math.min(remaining, mBufferSize));
                    }
                    bytesTransfered = mLength;
                }
                return connection.getResponseCode() == 200 && bytesTransfered == mLength;
            } finally {
                connection.disconnect();
            }
        }

        // GET or POST to one particular URL using URL.openConnection()
        private class SystemHttpURLConnectionFetchTask implements Callable<Boolean> {
            private final byte[] mBuffer = new byte[mBufferSize];

            @Override
            public Boolean call() {
                try {
                    return exerciseHttpURLConnection(mUrl.openConnection(), mBuffer);
                } catch (IOException e) {
                    System.out.println("System HttpURLConnection failed with " + e);
                    return false;
                }
            }
        }

        // GET or POST to one particular URL using Cronet HttpURLConnection API
        private class CronetHttpURLConnectionFetchTask implements Callable<Boolean> {
            private final byte[] mBuffer = new byte[mBufferSize];
            private final CronetHttpURLStreamHandler mStreamHandler;

            public CronetHttpURLConnectionFetchTask(
                    CronetHttpURLStreamHandler cronetStreamHandler) {
                mStreamHandler = cronetStreamHandler;
            }

            @Override
            public Boolean call() {
                try {
                    return exerciseHttpURLConnection(mStreamHandler.openConnection(mUrl), mBuffer);
                } catch (IOException e) {
                    System.out.println("Cronet HttpURLConnection failed with " + e);
                    return false;
                }
            }
        }

        // GET or POST to one particular URL using Cronet's asynchronous API
        private class CronetAsyncFetchTask implements Callable<Boolean> {
            // A message-queue for asynchronous tasks to post back to.
            private final LinkedBlockingQueue<Runnable> mWorkQueue =
                    new LinkedBlockingQueue<Runnable>();
            private final WorkQueueExecutor mWorkQueueExecutor = new WorkQueueExecutor();

            private int mRemainingRequests;
            private int mConcurrentFetchersDone;
            private boolean mFailed;

            CronetAsyncFetchTask() {
                mRemainingRequests = mIterations;
                mConcurrentFetchersDone = 0;
                mFailed = false;
            }

            private void initiateRequest(final ByteBuffer buffer) {
                if (mRemainingRequests == 0) {
                    mConcurrentFetchersDone++;
                    if (mUseNetworkThread) {
                        // Post empty task so message loop exit condition is retested.
                        postToWorkQueue(new Runnable() {
                            public void run() {}
                        });
                    }
                    return;
                }
                mRemainingRequests--;
                final Runnable completionCallback = new Runnable() {
                    public void run() {
                        initiateRequest(buffer);
                    }
                };
                final UrlRequest request = mCronetContext.createRequest(mUrl.toString(),
                        new Listener(buffer, completionCallback), mWorkQueueExecutor);
                if (mDirection == Direction.UP) {
                    request.setUploadDataProvider(new Uploader(buffer), mWorkQueueExecutor);
                    request.addHeader("Content-Type", "application/octet-stream");
                }
                request.start();
            }

            private class Uploader implements UploadDataProvider {
                private final ByteBuffer mBuffer;
                private int mRemainingBytes;

                Uploader(ByteBuffer buffer) {
                    mBuffer = buffer;
                    mRemainingBytes = mLength;
                }

                public long getLength() {
                    return mLength;
                }

                public void read(UploadDataSink uploadDataSink, ByteBuffer byteBuffer) {
                    mBuffer.clear();
                    // Don't post more than |mLength|.
                    if (mRemainingBytes < mBuffer.limit()) {
                        mBuffer.limit(mRemainingBytes);
                    }
                    // Don't overflow |byteBuffer|.
                    if (byteBuffer.remaining() < mBuffer.limit()) {
                        mBuffer.limit(byteBuffer.remaining());
                    }
                    byteBuffer.put(mBuffer);
                    mRemainingBytes -= mBuffer.position();
                    uploadDataSink.onReadSucceeded(false);
                }

                public void rewind(UploadDataSink uploadDataSink) {
                    uploadDataSink.onRewindError(new Exception("no rewinding"));
                }
            }

            private class Listener implements UrlRequestListener {
                private final ByteBuffer mBuffer;
                private final Runnable mCompletionCallback;

                Listener(ByteBuffer buffer, Runnable completionCallback) {
                    mBuffer = buffer;
                    mCompletionCallback = completionCallback;
                }

                @Override
                public void onResponseStarted(UrlRequest request, ResponseInfo info) {
                    mBuffer.clear();
                    request.read(mBuffer);
                }

                @Override
                public void onReceivedRedirect(
                        UrlRequest request, ResponseInfo info, String newLocationUrl) {
                    request.followRedirect();
                }

                @Override
                public void onReadCompleted(
                        UrlRequest request, ResponseInfo info, ByteBuffer byteBuffer) {
                    mBuffer.clear();
                    request.read(mBuffer);
                }

                @Override
                public void onSucceeded(UrlRequest request, ExtendedResponseInfo info) {
                    mCompletionCallback.run();
                }

                @Override
                public void onFailed(UrlRequest request, ResponseInfo info, UrlRequestException e) {
                    System.out.println("Async request failed with " + e);
                    mFailed = true;
                }
            }

            private void postToWorkQueue(Runnable task) {
                try {
                    mWorkQueue.put(task);
                } catch (InterruptedException e) {
                    mFailed = true;
                }
            }

            private class WorkQueueExecutor implements Executor {
                @Override
                public void execute(Runnable task) {
                    if (mUseNetworkThread) {
                        task.run();
                    } else {
                        postToWorkQueue(task);
                    }
                }
            }

            @Override
            public Boolean call() {
                // Initiate concurrent requests.
                for (int i = 0; i < mConcurrency; i++) {
                    initiateRequest(ByteBuffer.allocateDirect(mBufferSize));
                }
                // Wait for all jobs to finish.
                try {
                    while (mConcurrentFetchersDone != mConcurrency && !mFailed) {
                        mWorkQueue.take().run();
                    }
                } catch (InterruptedException e) {
                    System.out.println("Async tasks failed with " + e);
                    mFailed = true;
                }
                return !mFailed;
            }
        }

        /**
         * Executes the benchmark, times how long it takes, and records time in |mResults|.
         */
        public void run() {
            final ExecutorService executor = Executors.newFixedThreadPool(mConcurrency);
            final List<Callable<Boolean>> tasks = new ArrayList<Callable<Boolean>>(mIterations);
            startLogging();
            // Prepare list of tasks to run.
            switch (mMode) {
                case SYSTEM_HUC:
                    for (int i = 0; i < mIterations; i++) {
                        tasks.add(new SystemHttpURLConnectionFetchTask());
                    }
                    break;
                case CRONET_HUC: {
                    final CronetHttpURLStreamHandler cronetStreamHandler =
                            new CronetHttpURLStreamHandler(mCronetContext);
                    for (int i = 0; i < mIterations; i++) {
                        tasks.add(new CronetHttpURLConnectionFetchTask(cronetStreamHandler));
                    }
                    break;
                }
                case CRONET_ASYNC:
                    tasks.add(new CronetAsyncFetchTask());
                    break;
                default:
                    throw new IllegalArgumentException("Unknown mode: " + mMode);
            }
            // Execute tasks.
            boolean success = true;
            List<Future<Boolean>> futures = new ArrayList<Future<Boolean>>();
            try {
                startTimer();
                // If possible execute directly to lessen impact of thread-pool overhead.
                if (tasks.size() == 1 || mConcurrency == 1) {
                    for (int i = 0; i < tasks.size(); i++) {
                        if (!tasks.get(i).call()) {
                            success = false;
                        }
                    }
                } else {
                    futures = executor.invokeAll(tasks);
                    executor.shutdown();
                    executor.awaitTermination(240, TimeUnit.SECONDS);
                }
                stopTimer();
                for (Future<Boolean> future : futures) {
                    if (!future.isDone() || !future.get()) {
                        success = false;
                        break;
                    }
                }
            } catch (Exception e) {
                System.out.println("Batch execution failed with " + e);
                success = false;
            }
            stopLogging();
            if (success) {
                reportResult();
            }
        }
    }

    private class BenchmarkTask extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... unused) {
            JSONObject results = new JSONObject();
            for (Mode mode : Mode.values()) {
                for (Direction direction : Direction.values()) {
                    for (Protocol protocol : Protocol.values()) {
                        if (protocol == Protocol.QUIC && mode == Mode.SYSTEM_HUC) {
                            // Unsupported; skip.
                            continue;
                        }
                        // Run large and small benchmarks one at a time to test single-threaded use.
                        // Also run them four at a time to see how they benefit from concurrency.
                        // The value four was chosen as many devices are now quad-core.
                        new Benchmark(mode, direction, Size.LARGE, protocol, 1, results).run();
                        new Benchmark(mode, direction, Size.LARGE, protocol, 4, results).run();
                        new Benchmark(mode, direction, Size.SMALL, protocol, 1, results).run();
                        new Benchmark(mode, direction, Size.SMALL, protocol, 4, results).run();
                        // Large benchmarks are generally bandwidth bound and unaffected by
                        // per-request overhead.  Small benchmarks are not, so test at
                        // further increased concurrency to see if further benefit is possible.
                        new Benchmark(mode, direction, Size.SMALL, protocol, 8, results).run();
                    }
                }
            }
            final File outputFile = new File(getConfigString("RESULTS_FILE"));
            final File doneFile = new File(getConfigString("DONE_FILE"));
            // If DONE_FILE exists, something is horribly wrong, produce no results to convey this.
            if (doneFile.exists()) {
                results = new JSONObject();
            }
            // Write out results to RESULTS_FILE, then create DONE_FILE.
            FileOutputStream outputFileStream = null;
            FileOutputStream doneFileStream = null;
            try {
                outputFileStream = new FileOutputStream(outputFile);
                outputFileStream.write(results.toString().getBytes());
                outputFileStream.close();
                doneFileStream = new FileOutputStream(doneFile);
                doneFileStream.close();
            } catch (Exception e) {
                System.out.println("Failed write results file: " + e);
            } finally {
                try {
                    if (outputFileStream != null) {
                        outputFileStream.close();
                    }
                    if (doneFileStream != null) {
                        doneFileStream.close();
                    }
                } catch (IOException e) {
                    System.out.println("Failed to close output file: " + e);
                }
            }
            finish();
            return null;
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mConfig = getIntent().getData();
        // Execute benchmarks on another thread to avoid networking on main thread.
        new BenchmarkTask().execute();
    }
}
