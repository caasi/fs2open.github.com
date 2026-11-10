# Spike: IndexTTS-2.5 emotion vectors. Order: [happy, angry, sad, afraid, disgusted, melancholic, surprised, calm]
import time, torch, soundfile as sf
from indextts.infer_v2_5 import IndexTTS2
O = "/dev/shm/tts-idx/"; REF = "/dev/shm/tts-vox/out/"
tts = IndexTTS2(cfg_path="checkpoints/config.yaml", model_dir="checkpoints", use_bf16=True)
EMO = {"calm": [0,0,0,0,0,0,0,0.8], "urgent": [0,0.7,0,0.2,0,0,0.3,0], "fear": [0,0,0.2,0.8,0,0,0,0], "relief": [0.8,0,0,0,0,0,0,0]}
LINES = {
  "pilot": {"calm": ("阿尔法一号，前往跳跃点，等候进一步命令。","Alpha 1, proceeding to the jump node, awaiting further orders."),
            "urgent": ("他们在我后面！谁来帮我把他甩掉！","They're on my six! Somebody get him off me!"),
            "fear": ("护盾没了……引擎起火了，我撑不住了！","Shields are down... engines on fire, I can't hold it!"),
            "relief": ("目标摧毁！好样的，大家都活着回来了。","Target destroyed! Good work, everyone made it back.")},
  "command": {"calm": ("各中队注意，本次任务是护送运输舰通过跳跃点。","All squadrons, your mission is to escort the transport through the jump node."),
              "urgent": ("敌方轰炸机正在接近！所有战机立刻拦截！","Enemy bombers inbound! All fighters intercept now!"),
              "fear": ("那是……席瓦人的舰队？这不可能……","Is that... a Shivan fleet? That's impossible..."),
              "relief": ("干得好，飞行员们。运输舰安全抵达了。","Well done, pilots. The transport has arrived safely.")},
}
t0 = time.time()
for c, moods in LINES.items():
    for mood, (zh, en) in moods.items():
        for lang, text in (("ZH", zh), ("EN", en)):
            wav = O + f"idx-{c}-{mood}-{lang.lower()}.wav"
            tts.infer(spk_audio_prompt=REF + f"{c}-ref.wav", text=text, lang=lang, output_path=wav, emo_vector=EMO[mood], use_random=False)
            d, sr = sf.read(wav); sf.write(wav[:-4] + ".ogg", d, sr, format="OGG", subtype="VORBIS")
print("seconds:", round(time.time() - t0, 1), "max VRAM GB:", round(torch.cuda.max_memory_allocated() / 2**30, 2))
