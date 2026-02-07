#ifndef SPLITCHINESE_H
#define SPLITCHINESE_H
#if ARDUINO > 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

namespace SplitChinese
{

    enum CharType
    {
        TYPE_CHINESE, // 中文字符
        TYPE_ASCII    // 非中文字符
    };

    struct TextUnit
    {
        CharType type;   // 类型
        String content;  // 内容
        int originalPos; // 原始编号
    };

    class SplitChinese
    {
    private:
        // 提取所有中文字符
        String extractChinese(TextUnit *units, int count)
        {
            String chinese = "";
            for (int i = 0; i < count; i++)
            {
                if (units[i].type == TYPE_CHINESE)
                {
                    chinese += units[i].content;
                }
            }
            return chinese;
        }

        // 提取所有非中文字符
        String extractASCII(TextUnit *units, int count)
        {
            String ascii = "";
            for (int i = 0; i < count; i++)
            {
                if (units[i].type == TYPE_ASCII)
                {
                    ascii += units[i].content;
                }
            }
            return ascii;
        }

    public:
        void splitChineseAndASCII(String input, TextUnit *units, int &count)
        {
            count = 0; 
            int len = input.length();
            int pos = 0; // 原始位置索引

            while (pos < len)
            {
                // 获取当前字符的第一个字节
                byte b = input[pos];

                // 判断是否为中文（UTF-8编码：首字节 >= 0xE0 且 占3个字节）
                if (b >= 0xE0 && pos + 2 < len)
                {
                    int chineseStart = pos; // 中文片段起始位置
                    String chineseSeg = ""; // 合并后的中文片段
                    
                    while (pos < len && input[pos] >= 0xE0 && pos + 2 < len)
                    {
                        chineseSeg += input.substring(pos, pos + 3); 
                        pos += 3;                                    
                    }
                    units[count].type = TYPE_CHINESE;
                    units[count].content = chineseSeg;
                    units[count].originalPos = chineseStart;
                    count++;
                }
                else
                {
                    // 提取连续的ASCII字符（优化：合并连续非中文，减少单元数）
                    int asciiStart = pos;
                    while (pos < len && input[pos] < 0xE0)
                    {
                        pos++;
                    }
                    units[count].type = TYPE_ASCII;
                    units[count].content = input.substring(asciiStart, pos);
                    units[count].originalPos = asciiStart;
                    count++;
                }
            }
        }

        // 还原函数：从结构体数组还原原始字符串
        String restoreOriginalText(TextUnit *units, int count)
        {
            String result = "";
            // 简单冒泡排序
            for (int i = 0; i < count - 1; i++)
            {
                for (int j = 0; j < count - i - 1; j++)
                {
                    if (units[j].originalPos > units[j + 1].originalPos)
                    {
                        TextUnit temp = units[j];
                        units[j] = units[j + 1];
                        units[j + 1] = temp;
                    }
                }
            }
            // 拼接还原
            for (int i = 0; i < count; i++)
            {
                result += units[i].content;
            }
            return result;
        }
    };
}

#endif