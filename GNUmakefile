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
	cd xcode && xcodebuild -configuration Debug
xcode-release:xcode
	cd xcode && xcodebuild -configuration Release
xcode-release-install:
	cd xcode && xcodebuild -configuration Release -scheme install
-include Makefile
