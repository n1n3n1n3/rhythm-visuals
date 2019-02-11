import java.util.Collections;
import java.awt.Robot;
import java.io.IOException;

public class KeyboardMode extends Mode {
  Robot robot;

  public KeyboardMode() {
    //set defaults used by loadConfigFrom
    this.defaultConfig.setProperty("SCROLL_NOTE", "80");

    //sets loaded config
    loadConfigFrom("flock_config.properties");
    println("Flock config: ");
    println(this.loadedConfig);
  }

  public void setup() {
    try {
      robot = new Robot();
      robot.delay(1000);
    } catch (Exception e) {
      e.printStackTrace();
      exit();
    }
  }

  public void draw() {
    textSize(64);
    textAlign(CENTER);
    text("Scrolling", width/2, height/2 - 120);
  }

  public void handleMidi(byte[] raw, byte messageType, int channel, int note, int vel, int controllerNumber, int controllerVal, Pad pad) {
    if (note == this.getIntProp("SCROLL_NOTE") && vel > 0){
      robot.mouseWheel(1);
    }
  }
  
}