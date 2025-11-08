#!/usr/bin/python
import os
import subprocess
import sys
import traceback
import re

def get_lufs(file_path):
    """运行FFmpeg命令获取LUFS值并解析结果"""
    try:
        cmd = [
            'ffmpeg',
            '-i', file_path,
            '-filter_complex', 'ebur128=peak=true',
            '-f', 'null',
            '-'
        ]
        # 运行FFmpeg并捕获标准错误输出
        result = subprocess.run(
            cmd, 
            stderr=subprocess.PIPE, 
            check=False
        )
        # 使用错误处理方式解码输出
        stderr_output = result.stderr.decode('utf-8', errors='ignore')
        
        # 逐行处理输出
        for line in stderr_output.splitlines():
            # 跳过以[开头的行
            if line.strip().startswith('['):
                continue
            # 查找以"I:"开头的行
            if line.strip().startswith('I:'):
                parts = line.split()
                if len(parts) >= 3 and parts[0] == 'I:':
                    return parts[1]  # 返回LUFS值
        return None
    except Exception as e:
        traceback.print_exc()
        print(f"处理文件 {file_path} 时出错: {e}", file=sys.stderr)
        return None

compiled_reg = re.compile(r'.*\.((wav)|(mp3)|(ogg)|(aac)|(flac))$')


def scan_audio_files(root_dir):
    """递归扫描音频文件并处理"""
    for foldername, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if compiled_reg.match(filename.lower()):
                file_path = os.path.join(foldername, filename)
                yield filename, file_path

def usage():
    print('Usage:')
    print(f'    {sys.argv[0]} -r REGULAR_EXPRESSION')
    print(f'    {sys.argv[0]} -t TYPE1 TYPE2 ...')
    print(f'    {sys.argv[0]} -f MUSIC_FILE1 MUSIC_FILE2 ...')
    print(f'    {sys.argv[0]}')

def main():
    global compiled_reg
    file_list = []
    root_dir = os.path.abspath('.')
    if len(sys.argv) > 2:
        match_exp = None
        if sys.argv[1] == '-t':
            type_match = '|'.join([f'({suf})' for suf in sys.argv[2:]])
            match_exp = f'.*\\.({type_match})$'
        elif sys.argv[1] == '-r':
            match_exp = sys.argv[2]
        elif sys.argv[1] == '-f':
            file_list = [(os.path.basename(f), f) for f in sys.argv[2:]]
        else:
            usage()
            exit(1)
        if match_exp != None:
            compiled_reg = re.compile(match_exp)
            file_list = scan_audio_files(root_dir)
    elif len(sys.argv) == 1:
        file_list = scan_audio_files(root_dir)
    else:
        usage()
        exit(1)

    for filename, file_path in file_list:
        lufs = get_lufs(file_path)
        if lufs is not None:
            # 按要求的格式输出结果
            print(filename)
            print(f"path: {file_path}")
            print(f"lufs: {lufs}\n")

if __name__ == "__main__":
    main()
