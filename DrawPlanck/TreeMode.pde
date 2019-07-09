//grows trees as long as bpm stays in range
//most single tree growing branches code from https://www.openprocessing.org/sketch/717722
public class TreeMode extends Mode {

  private ArrayList<Branch> branch = new ArrayList<Branch>();
  private float offset = -90.0;
  private int count;
  private int s_color;
  private float s_weight;
  private boolean draw = false;

  public TreeMode() {
    this.modeName = "Arbres";
    
    //sets loaded config
    loadConfigFrom("tree_config.properties");
    println("tree config: ");
    println(loadedConfig);
  }

  public void setup() {
    System.out.println("MODE: Tree");
    pixelDensity(displayDensity());
    redrawBackground = false;
    background(200);
    colorMode(RGB, 255, 255, 255, 100);
    branch.add(new Branch(width / 2, height, width / 2, height - 80.0, 80.0, 0.0));
    count = 0;
    s_color = 0;
    s_weight = 0;
  }

  public void draw() {    
    if(draw) {
      for (int i = 0; i < branch.size() ; i++) {
        branch.get(i).Render();
        branch.get(i).Update();
      }
    }
  }

  public void handleMidi(byte[] raw, byte messageType, int channel, int note, int vel, int controllerNumber, int controllerVal, Pad pad) {    
    //filter out unassigned notes, note_off messages and unused pads
     if (pad != null && vel > 0) {
       System.out.println("pad detected " + pad.name + "\n branch size: " + branch.size());
       draw = true;
     } else {
       draw = false;
     }
  }

  private class Branch
  {
    float startx, starty, endx;
    float endy;
    float length;
    float degree;
    float nextx;
    float nexty;
    float prevx;
    float prevy;
    boolean next_flag = true;
    boolean draw_flag = true;

    public Branch(float sx, float sy, float ex, float ey, float sl, float sd)
    {
      startx = sx;
      starty = sy;
      endx = ex;
      endy = ey;
      length = sl;
      degree = sd;
      nextx = startx;
      nexty = starty;
      prevx = startx;
      prevy = starty;
      next_flag = true;
      draw_flag = true;
      Update();
      Render();
    }

    public void Update() {
      nextx += (endx - nextx) * 0.4;
      nexty += (endy - nexty) * 0.4;
      s_color = int (count / 10.0);
      s_weight = 3.0 / (count / 100 + 1);
      if (abs (nextx - endx) < 1.0 && abs (nexty - endy) < 1.0 && next_flag == true) {
        next_flag = false;
        draw_flag = false;
        nextx = endx;
        nexty = endy;
        int num = int (random (2, 4));
        for (int i = 0; i < num; i++) {
          float sx = endx;
          float sy = endy;
          float sl = random (random (10.0, 20.0), length * 0.99);
          float sd = random (-24.0, 24.0);
          float ex = sx + sl * cos (radians (sd + degree + offset));
          float ey = sy + sl * sin (radians (sd + degree + offset));
          branch.add(new Branch(sx, sy, ex, ey, sl, sd + degree));
        }
        count += 1;
      }
      if (branch.size() > 6000) {
        count = 0;
        s_color = 0;
        s_weight = 0;
        float sx = random (width);
        float sl = random (0.0, 180.0);
        branch = new ArrayList<Branch>();
        branch.add(new Branch(sx, height, sx, height - sl, sl, 0.0));
      }
    }

    public void Render() {
      if (draw_flag == true) {
        stroke (s_color);
        strokeWeight (s_weight);
        line (prevx, prevy, nextx, nexty);
      }
      prevx = nextx;
      prevy = nexty;
    }
  }
}
