using Godot;

public partial class main_menu : CanvasLayer
{
  [Export]
  private Button play_button;
  [Export]
  private Button exit_button;

  // Called when the node enters the scene tree for the first time.
  public override void _Ready()
  {
    GetTree().AutoAcceptQuit = true;
    play_button.Pressed += _on_play_button_pressed;
    exit_button.Pressed += _on_exit_button_pressed;
  }

  private void _on_play_button_pressed()
  {
    var loader = new LoadLevel(this, "res://levels/test_level.tscn");
  }

  private void _on_exit_button_pressed()
  {
    GetTree().Quit();
  }

  // Called every frame. 'delta' is the elapsed time since the previous frame.
  public override void _Process(double delta)
  {
  }
}
