using Godot;

public partial class test_level : Node3D
{
  [Export]
  private Button menu_button;
  // Called when the node enters the scene tree for the first time.
  public override void _Ready()
  {
    menu_button.Pressed += _on_menu_button_pressed;
  }

  private void _on_menu_button_pressed()
  {
    var loader = new LoadLevel(this, "res://levels/start.tscn");
  }

}
