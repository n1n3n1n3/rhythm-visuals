//grows trees as long as bpm stays in range
//most single tree growing branches code from https://www.openprocessing.org/sketch/717722
//Credit for the original script goes to OpenProcessing user Hoo Ja Oh
//License : https://creativecommons.org/licenses/by-sa/3.0/legalcode
public class TreeMode extends Mode {

  private ArrayList<Branch> branch;
  private float offset;
  private int count;
  private int s_color;
  private float s_weight;
  private boolean draw;
  //private int oldColor;
  //private boolean colorize;

  PImage vdj_logo;
  PImage ae_logo;
  PImage bg_image;
  long drawStart;

  static final int BRANCH_COLOR_CHANGE = 50;

  int Y_AXIS = 1;
  int X_AXIS = 2;
  color b1, b2, b3, b4;

  public TreeMode() {
    this.redrawBackground = false;
    this.modeName = "Reboisons! - Reforest!";

    //sets loaded config
    loadConfigFrom("tree_config.properties");
    println("tree config: ");
    println(loadedConfig);
  }

  //public void settings(){

  //}

  public void setup() {
    System.out.println("MODE: Tree");

    branch = new ArrayList<Branch>();
    offset = -90.0;
    //pixelDensity(displayDensity());
    colorMode(HSB, 255, 255, 255, 255);  

    //background(25, 180, 255);
    //background(121, 222, 255, 100);
    //background(0);

    bg_image = loadImage("Tree_bg.png");
    ae_logo = loadImage("ae.png");
    //vdj_logo = loadImage("bitmap_noir.png");
    //vdj_logo.resize((int) (vdj_logo.width * 0.33), 0);
    bg_image.resize(width, height);
    colorMode(RGB, 255, 255, 255, 100);
    branch.add(new Branch(width / 2, height, width / 2, height - 80.0, 80.0, 0.0));
    count = 0;
    s_color = 0;
    s_weight = 0;
    //colorize = false;
    draw = false;

    image(bg_image, 0, 0);
    image(ae_logo, 50, height - 300);
  }

  public void draw() {

    //colorMode(HSB, 255, 255, 255, 255);
    ////check if a pad was pressed
    //for (int i = 0; i < numPads; i++) {
    //  if (padWasPressed.get(i)) {
    //    this.resetPressed(i);
    //    colorize = true;
    //    break;
    //  }
    //}

    //if (colorize) {
    //  //Tint using bpm
    //  float constrainedBpm = constrain(currentBpm, 40, 150);
    //  int newColor = Math.round(map(constrainedBpm, 40, 150, 85, 0));

    //  //first time
    //  if (oldColor < 0) 
    //    oldColor = newColor;
    //  int newInterpolatedColor = Math.round(lerp(oldColor, newColor, 0.05));
    //  oldColor = newInterpolatedColor;
    //  fill(newInterpolatedColor, 255, 255, 2);
    //  noStroke();
    //  rectMode(CENTER);
    //  pushMatrix();
    //  translate(0, 0, -10);
    //  rect(width/2, height/2, width*2, height*2);
    //  popMatrix();
    //  rectMode(CORNER);
    //  colorize = false;
    //}

    imageMode(CENTER);
    //image(ae_logo, 2*width/5, height/4);
    //image(vdj_logo, 3*width/5, height/4);
    imageMode(CORNER);

    colorMode(RGB, 255, 255, 255, 100);
    if (draw || System.currentTimeMillis() - drawStart < 100) {
      for (int i = 0; i < branch.size(); i++) {
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
      drawStart = System.currentTimeMillis();
    } else {
      draw = false;
    }
  }

  private class Branch
  {
    float startx, starty, endx, endy;
    float length;
    float degree;
    float nextx, nexty;
    float prevx, prevy;
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
      nextx += (endx - nextx) * 0.65;
      nexty += (endy - nexty) * 0.65;
      s_color = int (count / 10.0);
      s_weight = 2.0 / (count / 100 + 1);
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
        float sy = height - random((int) (height/3));
        branch = new ArrayList<Branch>();
        branch.add(new Branch(sx, sy, sx, sy - sl, sl, 0.0));
      }
    }

    public void Render() {
      if (draw_flag == true) {
        if (branch.size() < BRANCH_COLOR_CHANGE)
          stroke(noisy(100), noisy(69), noisy(19));//stroke (s_color);
        else
          stroke(noisy(20), noisy(100), noisy(40));
        strokeWeight (s_weight);
        line (prevx, prevy, nextx, nexty);
      }
      prevx = nextx;
      prevy = nexty;
    }
  }

  private int noisy(int val) {
    int minRange = 15;
    return (int) (val + random(min(-val/4, -minRange), max(val/4, minRange)));
  }
}

void setGradient(int x, int y, float w, float h, color c1, color c2, int axis ) {

  noFill();

  if (axis == 1) {  // Top to bottom gradient
    for (int i = y; i <= y+h; i++) {
      float inter = map(i, y, y+h, 0, 1);
      color c = lerpColor(c1, c2, inter);
      stroke(c);
      line(x, i, x+w, i);
    }
  } else if (axis == 2) {  // Left to right gradient
    for (int i = x; i <= x+w; i++) {
      float inter = map(i, x, x+w, 0, 1);
      color c = lerpColor(c1, c2, inter);
      stroke(c);
      line(i, y, i, y+h);
    }
  }
}
