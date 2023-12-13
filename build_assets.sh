#!/bin/sh

# Enemy animations

3it to-cel -b 8 --coded true assets/graphics/flipper/flipper1.bmp -o assets/graphics/flipper/Flipper1.cel
mv assets/graphics/flipper/Flipper1.cel CD/Assets/Graphics/Flipper/

3it to-cel -b 8 --coded true assets/graphics/spiker/spiker1.bmp -o assets/graphics/spiker/Spiker1.cel
mv assets/graphics/spiker/Spiker1.cel CD/Assets/Graphics/Spiker/

3it to-cel -b 8 --coded true assets/graphics/Missile/missile1.bmp -o assets/graphics/Missile/Missile1.cel
mv assets/graphics/Missile/Missile1.cel CD/Assets/Graphics/Missile/
3it to-cel -b 8 --coded true assets/graphics/Missile/missile2.bmp -o assets/graphics/Missile/Missile2.cel
mv assets/graphics/Missile/Missile2.cel CD/Assets/Graphics/Missile/
3it to-cel -b 8 --coded true assets/graphics/Missile/missile3.bmp -o assets/graphics/Missile/Missile3.cel
mv assets/graphics/Missile/Missile3.cel CD/Assets/Graphics/Missile/

3it to-cel -b 8 --coded true assets/graphics/tanker/tanker1.bmp -o assets/graphics/tanker/Tanker1.cel
mv assets/graphics/tanker/Tanker1.cel CD/Assets/Graphics/Tanker/
3it to-cel -b 8 --coded true assets/graphics/tanker/tanker2.bmp -o assets/graphics/tanker/Tanker2.cel
mv assets/graphics/tanker/Tanker2.cel CD/Assets/Graphics/Tanker/
3it to-cel -b 8 --coded true assets/graphics/tanker/tanker3.bmp -o assets/graphics/tanker/Tanker3.cel
mv assets/graphics/tanker/Tanker3.cel CD/Assets/Graphics/Tanker/

3it to-cel -b 8 --coded true assets/graphics/fuseball/fuseball1.bmp -o assets/graphics/fuseball/Fuseball1.cel
mv assets/graphics/fuseball/Fuseball1.cel CD/Assets/Graphics/Fuseball/
3it to-cel -b 8 --coded true assets/graphics/fuseball/fuseball2.bmp -o assets/graphics/fuseball/Fuseball2.cel
mv assets/graphics/fuseball/Fuseball2.cel CD/Assets/Graphics/Fuseball/
3it to-cel -b 8 --coded true assets/graphics/fuseball/fuseball3.bmp -o assets/graphics/fuseball/Fuseball3.cel
mv assets/graphics/fuseball/Fuseball3.cel CD/Assets/Graphics/Fuseball/

3it to-cel -b 8 --coded true assets/graphics/pulsar/pulsar1.bmp -o assets/graphics/pulsar/Pulsar1.cel
mv assets/graphics/pulsar/Pulsar1.cel CD/Assets/Graphics/Pulsar/

3it to-cel -b 8 --coded true assets/graphics/ftanker/ftanker1.bmp -o assets/graphics/ftanker/Ftanker1.cel
mv assets/graphics/ftanker/Ftanker1.cel CD/Assets/Graphics/Ftanker/
3it to-cel -b 8 --coded true assets/graphics/ftanker/ftanker2.bmp -o assets/graphics/ftanker/Ftanker2.cel
mv assets/graphics/ftanker/Ftanker2.cel CD/Assets/Graphics/Ftanker/
3it to-cel -b 8 --coded true assets/graphics/ftanker/ftanker3.bmp -o assets/graphics/ftanker/Ftanker3.cel
mv assets/graphics/ftanker/Ftanker3.cel CD/Assets/Graphics/Ftanker/

3it to-cel -b 8 --coded true assets/graphics/ptanker/ptanker1.bmp -o assets/graphics/ptanker/Ptanker1.cel
mv assets/graphics/ptanker/Ptanker1.cel CD/Assets/Graphics/Ptanker/
3it to-cel -b 8 --coded true assets/graphics/ptanker/ptanker2.bmp -o assets/graphics/ptanker/Ptanker2.cel
mv assets/graphics/ptanker/Ptanker2.cel CD/Assets/Graphics/Ptanker/
3it to-cel -b 8 --coded true assets/graphics/ptanker/ptanker3.bmp -o assets/graphics/ptanker/Ptanker3.cel
mv assets/graphics/ptanker/Ptanker3.cel CD/Assets/Graphics/Ptanker/

# Enemy animations end

# points

3it to-cel -b 8 --coded true assets/graphics/points/p50.bmp -o assets/graphics/points/P50.cel
mv assets/graphics/points/P50.cel CD/Assets/Graphics/

3it to-cel -b 8 --coded true assets/graphics/points/p55.bmp -o assets/graphics/points/P55.cel
mv assets/graphics/points/P55.cel CD/Assets/Graphics/

3it to-cel -b 8 --coded true assets/graphics/points/p100.bmp -o assets/graphics/points/P100.cel
mv assets/graphics/points/P100.cel CD/Assets/Graphics/

3it to-cel -b 8 --coded true assets/graphics/points/p150.bmp -o assets/graphics/points/P150.cel
mv assets/graphics/points/P150.cel CD/Assets/Graphics/

3it to-cel -b 8 --coded true assets/graphics/points/p200.bmp -o assets/graphics/points/P200.cel
mv assets/graphics/points/P200.cel CD/Assets/Graphics/

3it to-cel -b 8 --coded true assets/graphics/points/p250.bmp -o assets/graphics/points/P250.cel
mv assets/graphics/points/P250.cel CD/Assets/Graphics/

# points end 

# game ui

3it to-cel -b 16 --coded false assets/graphics/ui/watch.bmp -o assets/graphics/ui/Watch.cel
mv assets/graphics/ui/Watch.cel CD/Assets/Graphics/UI/

3it to-cel -b 16 --coded false assets/graphics/ui/zapper.bmp -o assets/graphics/ui/Zapper.cel
mv assets/graphics/ui/Zapper.cel CD/Assets/Graphics/UI/

3it to-cel -b 8 --coded true assets/graphics/ui/digits.bmp -o assets/graphics/ui/Digits.cel
mv assets/graphics/ui/Digits.cel CD/Assets/Graphics/UI/

3it to-cel -b 8 --coded true assets/graphics/ui/over.bmp -o assets/graphics/ui/Over.cel
mv assets/graphics/ui/Over.cel CD/Assets/Graphics/UI/

3it to-cel -b 8 --coded true assets/graphics/ui/end.bmp -o assets/graphics/ui/End.cel
mv assets/graphics/ui/End.cel CD/Assets/Graphics/UI/

3it to-cel -b 8 --coded true assets/graphics/effects/exp.bmp -o assets/graphics/ui/Exp.cel
mv assets/graphics/ui/Exp.cel CD/Assets/Graphics/Effects/

# game ui end 

# title screen 

TP="assets/graphics/ui/title"

3it to-cel -b 16 --coded false --packed true $TP/play.bmp -o $TP/Play.cel
3it to-cel -b 16 --coded false --packed true $TP/options.bmp -o $TP/Options.cel
3it to-cel -b 16 --coded false --packed true $TP/logo.bmp -o $TP/Logo.cel
3it to-cel -b 16 --coded false --packed true $TP/credits.bmp -o $TP/Credits.cel

# join title cels

cat $TP/Logo.cel $TP/Credits.cel $TP/Play.cel $TP/Options.cel > $TP/THUD.cel
rm -f $TP/Logo.cel $TP/Credits.cel $TP/Play.cel $TP/Options.cel
mv $TP/THUD.cel CD/Assets/Graphics/UI/

# options screen

TP="assets/graphics/ui/options"

3it to-cel -b 16 --coded false --packed true $TP/on.bmp -o $TP/On.cel
3it to-cel -b 16 --coded false --packed true $TP/off.bmp -o $TP/Off.cel
3it to-cel -b 16 --coded false --packed true $TP/exit.bmp -o $TP/Exit.cel
3it to-cel -b 16 --coded false --packed true $TP/music.bmp -o $TP/Music.cel
3it to-cel -b 16 --coded false --packed true $TP/mode.bmp -o $TP/Mode.cel
3it to-cel -b 16 --coded false --packed true $TP/normal.bmp -o $TP/Normal.cel
3it to-cel -b 16 --coded false --packed true $TP/hard.bmp -o $TP/Hard.cel
3it to-cel -b 16 --coded false --packed true $TP/mouse.bmp -o $TP/Mouse.cel
3it to-cel -b 16 --coded false --packed true $TP/low.bmp -o $TP/Low.cel
3it to-cel -b 16 --coded false --packed true $TP/med.bmp -o $TP/Med.cel
3it to-cel -b 16 --coded false --packed true $TP/high.bmp -o $TP/High.cel

# join options cels

cat $TP/On.cel $TP/Off.cel $TP/Exit.cel $TP/Music.cel $TP/Mode.cel $TP/Normal.cel $TP/Hard.cel $TP/Low.cel $TP/Med.cel $TP/High.cel $TP/Mouse.cel > $TP/OHUD.cel
rm -f $TP/On.cel $TP/Off.cel $TP/Exit.cel $TP/Music.cel $TP/Mode.cel $TP/Normal.cel $TP/Hard.cel $TP/Low.cel $TP/Med.cel $TP/High.cel $TP/Mouse.cel 
mv $TP/OHUD.cel CD/Assets/Graphics/UI/

# end title screen

3it to-cel -b 8 --coded true assets/graphics/ui/life.bmp -o assets/graphics/ui/Life.cel
mv assets/graphics/ui/Life.cel CD/Assets/Graphics/UI/


# effects 

3it to-cel -b 8 --coded true assets/graphics/effects/zapped1.bmp -o assets/graphics/effects/Zapped1.cel
mv assets/graphics/effects/Zapped1.cel CD/Assets/Graphics/Effects/

3it to-cel -b 8 --coded true assets/graphics/effects/zapped2.bmp -o assets/graphics/effects/Zapped2.cel
mv assets/graphics/effects/Zapped2.cel CD/Assets/Graphics/Effects/

3it to-cel -b 8 --coded true assets/graphics/effects/zapped3.bmp -o assets/graphics/effects/Zapped3.cel
mv assets/graphics/effects/Zapped3.cel CD/Assets/Graphics/Effects/