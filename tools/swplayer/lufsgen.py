#!/usr/bin/python
import os
import subprocess
import sys
import traceback

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

def scan_audio_files(root_dir):
    """递归扫描音频文件并处理"""
    audio_exts = ('.wav', '.mp3', '.ogg', '.aac')
    for foldername, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.lower().endswith(audio_exts):
                file_path = os.path.join(foldername, filename)
                yield filename, file_path

def main():
    root_dir = os.path.abspath('.')  # 获取当前目录的绝对路径
    for filename, file_path in scan_audio_files(root_dir):
        lufs = get_lufs(file_path)
        if lufs is not None:
            # 按要求的格式输出结果
            print(filename)
            print(f"path: {file_path}")
            print(f"lufs: {lufs}\n")

if __name__ == "__main__":
    main()
