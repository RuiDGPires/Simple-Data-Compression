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

class BitWritter{
    private:
        FILE *file = NULL;
        u8 buffer = 0; 
        u8 buffer_size = 0;
    public:
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

        void write(std::vector<u8> bits){
            for (size_t i = 0; i < bits.size(); i++){
                put_bit(bits[i]);
            }
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

    std::map<u32, std::vector<u8>> table;


    FILE *fin = fopen(fin_name, "rb");
    BitWritter bw(fout_name);

    u8 buffer[BUFFER_SIZE];
    size_t n = 0;

    std::vector<u8> current;
    table[0] = current;
    u32 size = 0;
    u32 addr = 0;
    u32 count = 0;
    bool write_bit = false;

    while((n = fread(buffer, sizeof(u8), BUFFER_SIZE, fin)) != 0){
        for (size_t i = 0; i < n; i++){
            for (int j = 7; j >= 0; j--){
                u8 bit = ((buffer[i] & 0xff) >> j) & 0x01;

                size++;
                addr = (addr << 1) | bit;
                    
                if (size == dic_size){
                    size = 0;
                    current = table[addr];
                    write_bit = true;
                }else if (write_bit){
                    size = 0;
                    addr = 0;
                    current.push_back(bit);
                    bw.write(current);
                    write_bit = false;
                    if (count < table_size)
                        table[++count] = current;
                }
            }
        }
    }

    if (!current.empty()){
        u8 bit = current.back() & 0x01;
        current.pop_back();
    }

    fclose(fin);

    return 0;
}
