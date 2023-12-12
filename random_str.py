import random
import string

# 生成随机字符序列


def generate_random_sequence(length):
    # 包含所有英文字母和数字的字符串
    char_set = string.ascii_letters + string.digits

    # 生成随机字符序列
    random_chars = ''.join(random.choices(char_set, k=length))
    return random_chars

# 生成长度为200的随机字符序列


random_sequence = generate_random_sequence(10000)
# 将结果保存到文本文件
with open('random_sequence.txt', 'w') as file:
    # file.write(random_sequence)
    for i in range(1, 17):
        print1 = "close t" + str(i)
        file.write(print1)
        file.write('\n')

print("随机字符序列已保存到 random_sequence.txt 文件中。")
