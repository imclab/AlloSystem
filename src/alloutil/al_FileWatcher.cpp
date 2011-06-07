#include "alloutil/al_FileWatcher.hpp"

using namespace al;


/// Singleton object to manage filewatching notifications
/// Starts polling in the MainLoop scheduler as soon as FileWatcher::get() is called.
class FileWatcherManager {
public:
	struct WatchedFile {
		WatchedFile(std::string path, FileWatcher * handler, bool immediate)
		: mPath(path), mHandler(handler) { 
			if (immediate) {
				mModified = 0;
			} else {
				mModified = File::modified(path);
			}

		}
		WatchedFile(const WatchedFile& cpy)
		: mPath(cpy.mPath), mHandler(cpy.mHandler), mModified(cpy.mModified) {}
		
		std::string mPath;
		al_sec mModified;
		FileWatcher * mHandler;
	};
	typedef std::vector<WatchedFile> WatcherMap;

	/// access the singleton:
	static FileWatcherManager& get() {
		static FileWatcherManager singleton;
		return singleton;
	}
	
	FileWatcherManager() 
	:	mPeriod(1.),
		mActive(false)
	{
		// start polling:
		poll(MainLoop::now());
	}
	
	~FileWatcherManager() {}
	
	/// add a file to watch:
	/// this may overwrite an existing handler.
	/// param[in] immediate: triggers the handler immediately if the file exists
	FileWatcherManager& watch(std::string path, FileWatcher * handler, bool immediate) {
		if (al::fileExists(path)) {
			// note this may overwrite an existing handler.
			mHandlers.push_back(WatchedFile(path, handler, immediate));
		} else {
			printf("warning: attempt to watch non-existent file %s\n", path.c_str());
		}
		return *this;
	}
	
	/// stop watching a file:
	FileWatcherManager& unwatch(FileWatcher * handler) {
		// one watcher may handle several files:
		WatcherMap::iterator iter = mHandlers.begin();
		while (iter != mHandlers.end()) {
			WatchedFile& wf = *iter++;
			if (handler == wf.mHandler) mHandlers.erase(iter);
		}
		return *this;
	}
	
	/// set/get polling period:
	FileWatcherManager& period(al_sec p) {
		p = fabs(p);
		p = p < 0.01 ? 0.01 : p;
		mPeriod = p;
		return *this;
	}
	al_sec period() { return mPeriod; }
	
	// scheduler task function
	void poll(al_sec t) {
		WatcherMap::iterator iter = mHandlers.begin();
		while (iter != mHandlers.end()) {
			WatchedFile& wf = *iter++;
			File f(wf.mPath, "r", false);
			if (al::fileExists(wf.mPath)) {
				al_sec mod = f.modified();
				if(mod > wf.mModified) {
					f.open();
					wf.mHandler->onFileWatch(f);
					f.close();
					wf.mModified = f.modified();
				}
			}
		}
		MainLoop::queue().send(t+mPeriod, this, &FileWatcherManager::poll);
	}
	
	WatcherMap mHandlers;
	al_sec mPeriod;
	bool mActive;
};



FileWatcher::~FileWatcher(){ 
	FileWatcherManager::get().unwatch(this); 
}

void FileWatcher::watch(std::string filepath, bool immediate) {
	FileWatcherManager::get().watch(filepath, this, immediate); 
}

void FileWatcher::period(al_sec p) { FileWatcherManager::get().period(p); }
al_sec FileWatcher::period() { return FileWatcherManager::get().period(); }
