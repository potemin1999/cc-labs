/**
 * Created by ilya on 9/17/19.
 */

#include "Calculator.h"

char *readTemplate() {
    auto source = "1+(26-98)/15+777<28";
    auto result = new char[strlen(source) + 1];
    strcpy(result, source);
    return result;
}

char *readFile() {
    std::printf("Write file name: ");
    char fileName[256];
    std::scanf("%s", fileName);
    FILE *file = std::fopen(fileName, "rb");
    if (!file) {
        std::printf("Unable to read file\n");
        exit(127);
    }
    std::fseek(file, 0, SEEK_END);
    auto fileSize = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    auto charBuffer = new char[fileSize + 1];
    charBuffer[fileSize] = 0;
    std::fread(charBuffer, 1, fileSize, file);
    std::fclose(file);
    return charBuffer;
}

char *readStdin() {
    auto buffer = new char[4096];
    std::memset(buffer, 0, 4096);
    std::fscanf(stdin, "%s", buffer);
    return buffer;
}

int main(int argc, const char **argv) {
    char *str = nullptr;
    std::printf("Do you want \n"\
    "\tdemo from lab slides (1),\n"\
    "\tfile source (2)\n"\
    "\tstdin source (3)\n"
                "? : ");
    auto option = 0;
    while (true) {
        char buffer[128], *end;
        std::scanf("%s", buffer);
        option = std::strtol(buffer, &end, 10);
        if (option < 1 || option > 3) {
            std::printf("Enter correct number: ");
            continue;
        }
        break;
    }

    switch (option) {
        case 1: {
            str = readTemplate();
            break;
        }
        case 2: {
            str = readFile();
            break;
        }
        case 3: {
            str = readStdin();
            break;
        }
        default: {
            std::printf("Impossible...");
            std::fclose(nullptr);
        }
    }

    std::printf("Processing %s\n", str);
    Lexer lexer(str);
    Parser parser(lexer);
    Expression *expr = parser.parse();
    delete[] str;
    Value value = Calculator::calculate(expr);
    std::printf("Result = %d", value);
    return 0;
}