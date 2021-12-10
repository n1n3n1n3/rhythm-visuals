/**
Nina edit
TODO::
make a mode that loads the next mode in the setup.
Make it so that the mode loads the oncoming mode in setup by calling the appropriate setup function
*/

public class LoadingMode extends Mode {
  private PImage bg_img;
  private int timer;

  public LoadingMode() {
    this.modeName = "Loading - chargement";

    //sets loaded config
    this.loadConfigFrom("about_config.properties");
    println("c...");
    println(this.loadedConfig);
  }

  public void setup() {
    System.out.println("MODE: Loading");
    this.bg_img = loadImage("about.png");
    this.bg_img.resize(width, 0);      
    if (this.bg_img.height > height) {
      this.bg_img.resize(0, height);
    }
  }

  public void draw() {
    this.noModePressChecking();
    imageMode(CENTER);
    image(this.bg_img, width/2, height/2);
    imageMode(CORNERS);
  }

  public void handleMidi(byte[] raw, byte messageType, int channel, int note, int vel, int controllerNumber, int controllerVal, Pad pad) {
  }
}
