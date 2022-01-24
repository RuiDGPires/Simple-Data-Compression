#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <map>
#include <vector>
#include <string>

typedef uint32_t u32;
typedef uint8_t u8;

#define ASSERT(expr, ...){\
    if (!(expr)){\
        printf(__VA_ARGS__);\
        exit(1);\
    }\
}

#define BUFFER_SIZE 128
#define DEFAULT_DIC_SIZE 9

#define USAGE "./simplecompression <input file> <output file> [-d <dictionary size (bits)>]"

size_t get_file_size(FILE *file){
  // Missing error checking
  fseek(file, 0, SEEK_END); 
  size_t size = ftell(file); 
  fseek(file, 0, SEEK_SET); 
  return size;
}

class BitWritter{
    private:
        u8 buffer = 0; 
        u8 buffer_size = 0;
    public:
        FILE *file = NULL;
        BitWritter(){}
        ~BitWritter(){this->flush();fclose(this->file);}

        BitWritter(std::string file_name) {this->file = fopen(file_name.c_str(), "wb");}

        void put_bit(u8 bit){
            buffer = (buffer << 1) | (bit & 0x01);
            buffer_size++;

            if (buffer_size == 8){
                this->flush();
            }
        }

        void write(u32 dic_size, u32 addr, u8 bit){
            for (size_t i = 0; i < dic_size; i++){
                put_bit(addr >> (dic_size - i - 1));
            }

            put_bit(bit);
        }

        void flush(){
            if (buffer_size != 0){
                buffer <<= 8 - buffer_size;
                fwrite(&buffer, 1, 1, this->file);
                buffer_size = 0;
                buffer = 0;
            }
        }
};


void print_vec(std::vector<u8> vec){
    for (size_t i = 0; i < vec.size(); i++)
        printf("%d", vec[i]);
}

void print_bin(u32 val, int size){
    for (int i = size - 1; i >= 0; i--)
        printf("%d", (val >> i) & 0x01);
}

u32 append_bit(u32 val, u8 bit){
    return (val << 1) | (bit & 0x01);
}

u8 get_leading_bit_pos(u32 val){
    if (val == 0) return 0;

    u32 aux = val;
    u8 last_1 = 0;
    u8 pos = 0;

    for (u8 pos = 0; aux != 0; pos++){
        if (aux & 0x1){
            last_1 = pos;
        }
        aux >>= 1;
    }

    return last_1;
}

u32 remove_leading_bit(u32 val){
    u8 pos = get_leading_bit_pos(val);
    return val & ~(1 << pos);
}


int main(int argc, char *argv[]){
    ASSERT(argc >= 3, "%s\n", USAGE);

    char *fin_name = argv[1], *fout_name = argv[2];
    int dic_size = DEFAULT_DIC_SIZE; // in bits

    for (int i = 3; i < argc; i++){
        if (strcmp(argv[i], "-d") == 0){
            ASSERT(argc > i + 1, "%s\n", USAGE);
            
            int aux = atoi(argv[++i]) ;
            ASSERT(aux != 0 && aux <= 32, "Invalid dictionary size");
            dic_size = aux;
        }else{
            printf("%s\n", USAGE);
            exit(1);
        }

    }

    const size_t table_size = (int) pow(2.0, (double) (dic_size));

    std::map<std::vector<u8>, u32> table;


    FILE *fin = fopen(fin_name, "rb");
    BitWritter bw(fout_name);


    u8 buffer[BUFFER_SIZE];
    size_t n = 0;

    std::vector<u8> current;
    u32 count = 0;

    while((n = fread(buffer, sizeof(u8), BUFFER_SIZE, fin)) != 0){
        for (size_t i = 0; i < n; i++){
            for (int j = 7; j >= 0; j--){
                u8 bit = ((buffer[i] & 0xff) >> j) & 0x01;

                std::vector<u8> aux = current;
                aux.push_back(bit);

                if (table.count(aux) == 0){
                    if (count < table_size - 1)
                        table[aux] = ++count; 

                    bw.write(dic_size, table[current], bit);

                    current.clear();
                    continue;
                }

                current = aux;
            }
        }
    }

    if (!current.empty()){
        u8 bit = current.back() & 0x01;
        current.pop_back();
        bw.write(dic_size, table[current], bit);
    }

    bw.flush();
    size_t original_file_size = get_file_size(fin);
    size_t compressed_file_size = get_file_size(bw.file);

    double perc = 1 - (double) compressed_file_size / original_file_size;

    printf("%s (%dKB) -> %s (%dKB) | Compressed %0.2f\%\n", fin_name, original_file_size >> 10, fout_name, compressed_file_size >> 10, perc * 100);

    fclose(fin);

    return 0;
}
