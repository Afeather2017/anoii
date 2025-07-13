// 应用状态
var state = {
  playlists: {},                // 所有歌单数据
  currentPlaylist: null,        // 当前选中的歌单名称
  currentSongs: [],             // 当前显示的歌曲列表
  currentIndex: -1,             // 当前播放歌曲的索引
  playedSongIndexes: new Set(), // 播放过的歌，用于实现不重复的随机播放
  playing: false,               // 是否正在播放
  playMode: 'sequential',       // 播放模式: sequential(顺序)/random(随机)
  timer: null,                  // 定时器
  searchResults: [],            // 搜索结果
  volumeState: 'auto',          // 音量设置模式
  fixVolume: -27,               // 固定音量(lufs)
  manualVolume: 0.5,            // 手动设置音量
  timerActive: false,           // 定时器是否开启
  timerRemaining: 0,            // 定时器剩余时间
  timerInterval: null,
};

const volumeModes = ['auto', 'manual', 'fixed'];
const modeLabels = {
  'auto': '自动音量平衡',
  'manual': '手动设置音量',
  'fixed': '固定音量大小',
};

// DOM元素
const elements = {
  // 视图容器
  playlistsView: document.getElementById('playlists-view'),
  songsView: document.getElementById('songs-view'),
  middleContainer: document.getElementById('middle-container'),
  searchResultsView: document.getElementById('search-results-view'),

  // 列表容器
  playlistsList: document.getElementById('playlists'),
  songsList: document.getElementById('songs'),
  searchResultsList: document.getElementById('search-results'),

  // 按钮
  backButton: document.getElementById('back-button'),
  searchBackButton: document.getElementById('search-back-button'),
  searchButton: document.getElementById('search-button'),
  settingsButton: document.getElementById('settings-button'),
  playButton: document.getElementById('play-button'),
  prevButton: document.getElementById('prev-button'),
  nextButton: document.getElementById('next-button'),
  modeButton: document.getElementById('mode-button'),
  timerButton: document.getElementById('timer-button'),
  confirmTimer: document.getElementById('confirm-timer'),
  cancelTimer: document.getElementById('cancel-timer'),

  // 其他元素
  searchInput: document.getElementById('search-input'),
  progressBar: document.getElementById('progress-bar'),
  progress: document.getElementById('progress'),
  currentTime: document.getElementById('current-time'),
  playingSongName: document.getElementById('playing-song-name'),
  totalTime: document.getElementById('total-time'),
  audioPlayer: document.getElementById('audio-player'),
  currentPlaylistTitle: document.getElementById('current-playlist'),
  searchResultsTitle: document.getElementById('search-results-title'),

  // 设置对话框
  settingsDialog: document.getElementById('settings-dialog'),
  settingsButton: document.getElementById('settings-button'),
  volumeModeToggle: document.getElementById('volume-mode-toggle'),
  volumeModeValue: document.getElementById('volume-mode-value'),
  manualPanel: document.getElementById('manual-panel'),
  fixedPanel: document.getElementById('fixed-panel'),
  volumeSlider: document.getElementById('volume-slider'),
  volumeInput: document.getElementById('volume-input'),
  lufsSlider: document.getElementById('lufs-slider'),
  lufsInput: document.getElementById('lufs-input'),
  settingsOk: document.getElementById('settings-ok'),
};

// 初始化应用
function init() {
  // 获取歌单数据
  fetchPlaylists();

  // 添加事件监听器
  setupEventListeners();

  // 添加定时事件监听
  setupTimerListeners();
}

function setTimer(minutes) {
  if (minutes > 0) {
    // 清除现有定时器
    if (state.timerInterval) {
      clearInterval(state.timerInterval);
    }

    // 设置定时器
    state.timerActive = true;
    state.timerRemaining = minutes * 60; // 转换为秒

    // 更新状态显示
    updateTimerStatus();

    // 启动定时器
    state.timerInterval = setInterval(() => {
      state.timerRemaining--;

      if (state.timerRemaining <= 0) {
        // 时间到，停止播放
        clearInterval(state.timerInterval);
        state.timerActive = false;
        if (state.playing) {
          togglePlayPause();
        }
        updateTimerStatus();
        alert('定时播放已结束');
      } else {
        // 更新状态显示
        updateTimerStatus();
      }
    }, 1000);
  }
}

// 添加定时相关的事件监听
function setupTimerListeners() {
  // 定时滑动条和输入框同步
  const timerSlider = document.getElementById('timer-slider');
  const timerInput = document.getElementById('timer-input');

  timerSlider.addEventListener('input', () => {
    timerInput.value = timerSlider.value;
  });

  timerInput.addEventListener('input', () => {
    let value = Math.min(360, Math.max(0, parseInt(timerInput.value) || 0));
    timerInput.value = value;
    timerSlider.value = value;
  });

  // 预设时间按钮
  document.querySelectorAll('.timer-preset').forEach(button => {
    button.addEventListener('click', () => {
      const minutes = parseInt(button.dataset.minutes);
      timerSlider.value = minutes;
      timerInput.value = minutes;
      setTimer(minutes);
    });
  });

  // 开始定时按钮
  document.getElementById('start-timer').addEventListener('click', () => {
    setTimer(parseInt(timerInput.value));
  });

  // 取消定时按钮
  document.getElementById('cancel-timer').addEventListener('click', () => {
    // 清除现有定时器
    if (state.timerInterval) {
      clearInterval(state.timerInterval);
    }

    // 设置定时器
    state.timerActive = false;
    state.timerRemaining = 0; // 转换为秒
    updateTimerStatus();
  });
}


// 更新定时状态显示
function updateTimerStatus() {
  const timerStatus = document.getElementById('timer-status');

  if (state.timerActive) {
    const minutes = Math.floor(state.timerRemaining / 60);
    const seconds = state.timerRemaining % 60;
    timerStatus.textContent = `已定时: ${minutes}分${seconds.toString().padStart(2, '0')}秒后停止`;
  } else {
    timerStatus.textContent = '未启用定时';
  }
}

function fetchPlaylists() {
  fetch('/list_music')
    .then(response => response.json())
    .then(data => {
      state.playlists = data;
      renderPlaylists();
    })
    .catch(error => {
      console.error('获取歌单失败:', error);
      // 使用模拟数据
      state.playlists = {
        "歌单获取失败": [{ name: "歌单获取失败", lufs: 0.5 }],
      };
      renderPlaylists();
    });
}

function renderPlaylists() {
  elements.playlistsList.innerHTML = '';

  for (const [name, songs] of Object.entries(state.playlists)) {
    const playlistItem = document.createElement('li');
    playlistItem.className = 'playlist-item';
    playlistItem.innerHTML = `
      <div class="playlist-icon">♪</div>
      <div class="playlist-info">
        <div class="playlist-name">${name}</div>
        <div class="playlist-count">${songs.length}首歌曲</div>
      </div>
    `;

    playlistItem.addEventListener('click', () => {
      state.currentPlaylist = name;
      state.playedSongIndexes = new Set();
      state.currentSongs = [...songs]; // 存储完整歌曲对象
      console.log("recommandLufs set to", minLufs);
      elements.currentPlaylistTitle.textContent = name;
      renderSongs(songs);
      showSongsView();
    });

    elements.playlistsList.appendChild(playlistItem);
  }
}

// 渲染歌曲列表
function renderSongs(songs) {
  elements.songsList.innerHTML = '';

  songs.forEach((song, index) => {
    const songItem = document.createElement('li');
    songItem.className = 'song-item';

    // 从文件名中提取歌曲名称（移除扩展名）
    const songName = song.name.replace(/\.[^/.]+$/, "");

    songItem.innerHTML = `
      <div class="song-info">
        <div class="song-title">${songName}</div>
      </div>
    `;

    songItem.addEventListener('click', () => {
      playSong(song, index);
    });

    elements.songsList.appendChild(songItem);
  });
}

// 显示歌单视图
function showPlaylistsView() {
  elements.playlistsView.style.display = 'block';
  elements.songsView.style.display = 'none';
  elements.searchResultsView.style.display = 'none';
  elements.middleContainer.scrollTop = 0;
}

// 显示歌曲视图
function showSongsView() {
  elements.playlistsView.style.display = 'none';
  elements.songsView.style.display = 'block';
  elements.searchResultsView.style.display = 'none';
  elements.middleContainer.scrollTop = 0;
}

// 显示搜索结果视图
function showSearchResultsView() {
  elements.playlistsView.style.display = 'none';
  elements.songsView.style.display = 'none';
  elements.searchResultsView.style.display = 'block';
  elements.middleContainer.scrollTop = 0;
}

function setVolume() {
  if (state.currentSongs.length == 0)
    return;
  song = state.currentSongs[state.currentIndex];
  var volume = 0.5;
  if (state.volumeState == 'auto') {
    var minLufs = 1000;
    for (const s of state.currentSongs) {
      minLufs = Math.min(s.lufs, minLufs);
    }

    volume = 10 ** ((minLufs - song.lufs) / 20);
    console.log("minLufs=" + minLufs,
      "lufs=" + song.lufs,
      "volume=" + volume.toFixed(6));
  } else if (state.volumeState == 'fixed') {
    volume = 10 ** ((state.fixVolume - song.lufs) / 20);
    console.log("fixedLufs=" + state.fixVolume,
      "lufs=" + song.lufs,
      "volume=" + volume.toFixed(6));
  } else if (state.volumeState == 'manual') {
    volume = state.manualVolume;
    console.log("manualVolume=" + state.manualVolume);
  } else {
    // WTF??
  }
  // 设置音频播放器音量
  elements.audioPlayer.volume = Math.min(1, Math.max(0, volume));
  if (volume != elements.audioPlayer.volume) 
    console.log("volume cannot set, expect", volume, ", in fact",
      elements.audioPlayer.volume);
}

// 播放歌曲
function playSong(song, index) {
  // 更新当前播放索引
  state.currentIndex = index;
  state.playedSongIndexes.add(index);

  setVolume();

  // 设置音频源
  elements.audioPlayer.src = `/get_music?music-name=${encodeURIComponent(song.name)}`;

  // 显示歌曲名（不含扩展名）
  const songName = song.name.replace(/\.[^/.]+$/, "");
  elements.playingSongName.textContent = songName;

  // 播放音频
  elements.audioPlayer.play()
    .then(() => {
      state.playing = true;
      elements.playButton.textContent = '⏸';
    })
    .catch(error => {
      console.error('播放失败:', error);
    });
}

// 切换播放/暂停
function togglePlayPause() {
  if (state.playing) {
    elements.audioPlayer.pause();
    state.playing = false;
    elements.playButton.textContent = '▶';
  } else {
    elements.audioPlayer.play();
    state.playing = true;
    elements.playButton.textContent = '⏸';
  }
}

// 随机挑选歌曲，避免多次随机挑选到同一首歌曲
function randomSongIndexNoRepeat() {
  var notPlayed = state.currentSongs.length - state.playedSongIndexes.size;
  if (notPlayed == 0) {
    state.playedSongIndexes = new Set();
    return Math.floor(Math.random() * state.currentSongs.length);
  }
  var count = Math.ceil(Math.random() * notPlayed);
  for (i = 0; i < state.currentSongs.length; i++) {
    if (!state.playedSongIndexes.has(i))
      count--;
    if (count == 0)
      return i;
  }
}

// 播放上一首
function playPrevious() {
  if (state.currentSongs.length === 0 || state.currentIndex === -1) return;

  let newIndex;
  if (state.playMode === 'random') {
    // 随机播放模式
    newIndex = randomSongIndexNoRepeat();
  } else {
    // 顺序播放模式
    newIndex = state.currentIndex === 0 ? 
      state.currentSongs.length - 1 : 
      state.currentIndex - 1;
  }

  playSong(state.currentSongs[newIndex], newIndex);
}

// 播放下一首
function playNext() {
  if (state.currentSongs.length === 0 || state.currentIndex === -1) return;

  let newIndex;
  if (state.playMode === 'random') {
    // 随机播放模式
    newIndex = randomSongIndexNoRepeat();
  } else {
    // 顺序播放模式
    newIndex = state.currentIndex === state.currentSongs.length - 1 ? 
      0 : 
      state.currentIndex + 1;
  }

  playSong(state.currentSongs[newIndex], newIndex);
}

// 切换播放模式
function togglePlayMode() {
  if (state.playMode === 'sequential') {
    state.playMode = 'random';
    elements.modeButton.textContent = '⤮';
    elements.modeButton.classList.add('active');
  } else {
    state.playMode = 'sequential';
    elements.modeButton.textContent = '↻';
    elements.modeButton.classList.remove('active');
  }
}

// 获取歌单数据
function fetchPlaylists() {
  fetch('/list_music')
    .then(response => response.json())
    .then(data => {
      // 将数据转换为新格式：{歌单名: {name: 文件名, lufs: 音量}}
      state.playlists = data;
      renderPlaylists();
    })
    .catch(error => {
      console.error('获取歌单失败:', error);
      // 使用模拟数据
      state.playlists = {
        "歌单获取失败": [{ name: "歌单获取失败", lufs: 0.5 }],
      };
      renderPlaylists();
    });
}

// 渲染歌单列表
function renderPlaylists() {
  elements.playlistsList.innerHTML = '';

  for (const [name, songs] of Object.entries(state.playlists)) {
    const playlistItem = document.createElement('li');
    playlistItem.className = 'playlist-item';
    playlistItem.innerHTML = `
      <div class="playlist-icon">♪</div>
      <div class="playlist-info">
        <div class="playlist-name">${name}</div>
        <div class="playlist-count">${songs.length}首歌曲</div>
      </div>
    `;

    playlistItem.addEventListener('click', () => {
      state.currentPlaylist = name;
      state.playedSongIndexes = new Set();
      state.currentSongs = [...songs]; // 存储完整歌曲对象
      elements.currentPlaylistTitle.textContent = name;
      renderSongs(songs);
      showSongsView();
    });

    elements.playlistsList.appendChild(playlistItem);
  }
}

// 渲染歌曲列表
function renderSongs(songs) {
  elements.songsList.innerHTML = '';

  songs.forEach((song, index) => {
    const songItem = document.createElement('li');
    songItem.className = 'song-item';

    // 从文件名中提取歌曲名称（移除扩展名）
    const songName = song.name.replace(/\.[^/.]+$/, "");

    songItem.innerHTML = `
      <div class="song-info">
        <div class="song-title">${songName}</div>
        <div class="song-artist">未知艺术家</div>
      </div>
    `;

    songItem.addEventListener('click', () => {
      playSong(song, index);
    });

    elements.songsList.appendChild(songItem);
  });
}

// 搜索歌曲
function searchSongs() {
  const query = elements.searchInput.value.trim().toLowerCase();
  if (!query) return;

  // 清空搜索结果
  state.searchResults = [];
  elements.searchResultsList.innerHTML = '';

  // 搜索所有歌单
  for (const [playlistName, songs] of Object.entries(state.playlists)) {
    songs.forEach(song => {
      // 从文件名中提取歌曲名称（移除扩展名）
      const songName = song.name.replace(/\.[^/.]+$/, "").toLowerCase();

      if (songName.includes(query)) {
        state.searchResults.push({
          name: songName,
          name: song.name,
          lufs: song.lufs,
          playlist: playlistName
        });
      }
    });
  }

  // 渲染搜索结果
  if (state.searchResults.length > 0) {
    elements.searchResultsTitle.textContent = `找到${state.searchResults.length}个结果`;

    state.searchResults.forEach(item => {
      const resultItem = document.createElement('li');
      resultItem.className = 'song-item';
      resultItem.innerHTML = `
                        <div class="song-info">
                            <div class="song-title">${item.name}</div>
                            <div class="song-artist">来自: ${item.playlist}</div>
                        </div>
                    `;

      resultItem.addEventListener('click', () => {
        // 在搜索结果中播放歌曲
        state.currentSongs = state.searchResults.map(i => i.name);
        const index = state.searchResults.findIndex(i => i.name === item.name);
        playSong(item.name, index);
      });

      elements.searchResultsList.appendChild(resultItem);
    });

    showSearchResultsView();
  } else {
    elements.searchResultsTitle.textContent = "没有找到相关歌曲";
    showSearchResultsView();
  }
}


// 更新音量模式显示
function updateVolumeModeDisplay() {
  // 更新模式文本
  elements.volumeModeValue.textContent = modeLabels[state.volumeState];

  // 隐藏所有面板
  elements.manualPanel.classList.remove('active');
  elements.fixedPanel.classList.remove('active');

  // 显示当前模式的面板
  if (state.volumeState === 'manual') {
    elements.manualPanel.classList.add('active');
    // 同步滑块和输入框的值
    elements.volumeSlider.value = state.manualVolume;
    elements.volumeInput.value = state.manualVolume;
  } else if (state.volumeState === 'fixed') {
    elements.fixedPanel.classList.add('active');
    // 同步滑块和输入框的值
    elements.lufsSlider.value = state.fixVolume;
    elements.lufsInput.value = state.fixVolume;
  } else if (state.volumeState === 'auto') {
    // setVolume处理
  } else {
    // WTF??
  }
  setVolume();
}

// 设置事件监听器
function setupEventListeners() {
  // 返回按钮
  elements.backButton.addEventListener('click', showPlaylistsView);
  elements.searchBackButton.addEventListener('click', showPlaylistsView);

  // 搜索按钮
  elements.searchButton.addEventListener('click', searchSongs);
  elements.searchInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') searchSongs();
  });

  // 播放控制按钮
  elements.playButton.addEventListener('click', togglePlayPause);
  elements.prevButton.addEventListener('click', playPrevious);
  elements.nextButton.addEventListener('click', playNext);
  elements.modeButton.addEventListener('click', togglePlayMode);
  elements.settingsButton.addEventListener('click', () => {
    elements.settingsDialog.style.display = 'flex';
  });

  // 设置按钮
  elements.settingsButton.addEventListener('click', () => {
    elements.settingsDialog.style.display = 'flex';
    updateVolumeModeDisplay();
  });

  // 设置对话框的确认按钮
  elements.settingsOk.addEventListener('click', () => {
    elements.settingsDialog.style.display = 'none';
  });

  // 设置对话框的音量模式切换
  elements.volumeModeToggle.addEventListener('click', () => {
    const currentIndex = volumeModes.indexOf(state.volumeState);
    const nextIndex = (currentIndex + 1) % volumeModes.length;
    state.volumeState = volumeModes[nextIndex];
    updateVolumeModeDisplay();
  });

  // 设置对话框中，手动音量模式的输入框与滑块
  elements.volumeSlider.addEventListener('input', () => {
    state.manualVolume = elements.volumeSlider.value;
    updateVolumeModeDisplay();
  });

  elements.volumeInput.addEventListener('input', () => {
    let value = Math.min(1, Math.max(0, elements.volumeInput.value));
    state.manualVolume = value;
    updateVolumeModeDisplay();
  });

  // 设置对话框中，固定音量模式的输入框与滑块
  elements.lufsSlider.addEventListener('input', () => {
    state.fixVolume = elements.lufsSlider.value;
    updateVolumeModeDisplay();
  });

  elements.lufsInput.addEventListener('input', () => {
    let value = Math.min(0, Math.max(-100, elements.lufsInput.value));
    state.fixVolume = value;
    updateVolumeModeDisplay();
  });

  // 进度条控制
  elements.progressBar.addEventListener('click', (e) => {
    const rect = elements.progressBar.getBoundingClientRect();
    const percent = (e.clientX - rect.left) / rect.width;
    elements.audioPlayer.currentTime = percent * elements.audioPlayer.duration;
  });

  // 音频事件监听
  elements.audioPlayer.addEventListener('timeupdate', () => {
    // 更新进度条
    const percent = (elements.audioPlayer.currentTime / elements.audioPlayer.duration) * 100;
    elements.progress.style.width = `${percent}%`;

    // 更新时间显示
    elements.currentTime.textContent = formatTime(elements.audioPlayer.currentTime);
  });

  elements.audioPlayer.addEventListener('loadedmetadata', () => {
    elements.totalTime.textContent = formatTime(elements.audioPlayer.duration);
  });

  elements.audioPlayer.addEventListener('ended', playNext);
}

// 格式化时间显示
function formatTime(seconds) {
  const min = Math.floor(seconds / 60);
  const sec = Math.floor(seconds % 60);
  return `${min}:${sec.toString().padStart(2, '0')}`;
}

// 页面加载完成后初始化应用
window.addEventListener('DOMContentLoaded', init);
