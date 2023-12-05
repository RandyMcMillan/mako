-:
	cmake .
.PHONY:debug release xcode
debug:
	cmake . && cmake --build . --config Debug
release:
	cmake . && cmake --build . --config Release
xcode:-
	cmake -G Xcode -S . -B $@
xcode-debug:xcode
	cd xcode && xcodebuild -configuration Debug -arch x86_64 -arch arm64
xcode-release:xcode
	cd xcode && xcodebuild -configuration Release -arch x86_64 -arch arm64
xcode-release-install:
	cd xcode && xcodebuild -configuration Release -scheme install
	cd xcode && xcodebuild -configuration Release -scheme install -arch arm64
-include Makefile
