-:
	cmake .
.PHONY:debug release xcode
debug:
	cmake . -D BUILD_RELEASE=0 #&& cmake --build . --config Debug
debug-build:debug
	cmake --build . --config Debug
release:
	cmake . -D BUILD_RELEASE=1 #&& cmake --build . --config Release
release-build:
	cmake --build . --config Release
xcode:xcode-debug
xcode-debug:debug
	@cmake -D BUILD_RELEASE=0 -G Xcode -S . -B xcode
	cd xcode && xcodebuild -configuration Debug -scheme ALL_BUILD -arch x86_64 -arch arm64
xcode-debug-install:debug
	cd xcode && xcodebuild -configuration Debug -scheme install -arch arm64 -arch x86_64
xcode-release:release
	@cmake -D BUILD_RELEASE=1 -G Xcode -S . -B xcode
	cd xcode && xcodebuild -configuration Release -scheme ALL_BUILD -arch x86_64 -arch arm64
xcode-release-install:release
	cd xcode && xcodebuild -configuration Release -scheme install -arch arm64 -arch x86_64
-include Makefile
