ZIP_C_FILES = src/zip.cpp
UNZIP_C_FILES = src/unzip.cpp

all: zip unzip

zip: $(ZIP_C_FILES)
	g++ -g $(ZIP_C_FILES) -o $@ -lm

unzip:
	g++ -g $(UNZIP_C_FILES) -o $@ -lm

clean:
	rm zip unzip
