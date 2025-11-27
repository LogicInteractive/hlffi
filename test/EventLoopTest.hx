class EventLoopTest {
    public static function main() {
        trace("Testing EventLoop access");

        // Get current thread's event loop
        var loop = sys.thread.Thread.current().events;
        trace("EventLoop: " + loop);

        // Try to progress/process the loop
        if (loop != null) {
            trace("Processing events...");
            loop.progress();
            trace("Events processed");
        }
    }
}
