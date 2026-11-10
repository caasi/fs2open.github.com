# Spike: VoxCPM2. Design one neutral reference per character, then clone it with an emotion prefix.
import time, torch, soundfile as sf
from voxcpm import VoxCPM
O = "/dev/shm/tts-vox/out/"
m = VoxCPM.from_pretrained("openbmb/VoxCPM2", load_denoiser=False)
SR = m.tts_model.sample_rate
def gen(text, **kw):
    torch.manual_seed(7)
    return m.generate(text=text, cfg_value=2.0, inference_timesteps=10, **kw)
def save(name, wav):
    sf.write(O + name + ".ogg", wav, SR, format="OGG", subtype="VORBIS")

CHARS = {
    "pilot": "A male fighter pilot in his thirties, mid-range voice, speaking over a military radio",
    "command": "A female fleet commander in her forties, clear and authoritative voice, speaking over a military radio",
}
REF = "这里是指挥部，所有单位注意，保持通讯畅通。"
LINES = {
    "pilot": [("calm","calm and steady","阿尔法一号，前往跳跃点，等候进一步命令。","Alpha 1, proceeding to the jump node, awaiting further orders."),
              ("urgent","urgent, shouting in a dogfight","他们在我后面！谁来帮我把他甩掉！","They're on my six! Somebody get him off me!"),
              ("fear","terrified, voice shaking","护盾没了……引擎起火了，我撑不住了！","Shields are down... engines on fire, I can't hold it!"),
              ("relief","relieved, laughing a little","目标摧毁！好样的，大家都活着回来了。","Target destroyed! Good work, everyone made it back.")],
    "command": [("calm","calm, giving a briefing","各中队注意，本次任务是护送运输舰通过跳跃点。","All squadrons, your mission is to escort the transport through the jump node."),
                ("urgent","urgent, giving orders under attack","敌方轰炸机正在接近！所有战机立刻拦截！","Enemy bombers inbound! All fighters intercept now!"),
                ("fear","shocked and afraid","那是……席瓦人的舰队？这不可能……","Is that... a Shivan fleet? That's impossible..."),
                ("relief","proud and relieved","干得好，飞行员们。运输舰安全抵达了。","Well done, pilots. The transport has arrived safely.")],
}
t0 = time.time()
for c, desc in CHARS.items():
    ref = gen(f"({desc}, neutral tone){REF}")
    save(f"vox-{c}-ref", ref)
    sf.write(O + f"{c}-ref.wav", ref, SR)
    for tag, mood, zh, en in LINES[c]:
        save(f"vox-{c}-{tag}-zh", gen(f"({mood}){zh}", reference_wav_path=O + f"{c}-ref.wav"))
        save(f"vox-{c}-{tag}-en", gen(f"({mood}){en}", reference_wav_path=O + f"{c}-ref.wav"))
# Cantonese, explicit in the description
save("vox-command-cantonese", gen("(A female fleet commander, speaking Cantonese, calm)各中隊注意，今次任務係護送運輸艦通過跳躍點。"))
save("vox-pilot-cantonese", gen("(粤语，男性战机飞行员，紧张地大喊)佢哋喺我後面！邊個嚟幫我甩甩佢！"))
print("seconds:", round(time.time() - t0, 1), "max VRAM GB:", round(torch.cuda.max_memory_allocated() / 2**30, 2))
