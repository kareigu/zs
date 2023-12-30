using Godot;
using Godot.Collections;

public partial class LoadLevel : Node2D
{
  [Export]
  public string scene_to_load;

  private Array progress;

  private Node2D loading_screen;
  private ProgressBar progress_bar;

  public LoadLevel(Node parent, string scene_to_load)
  {
    this.scene_to_load = scene_to_load;
    parent.AddChild(this);
  }

  // Called when the node enters the scene tree for the first time.
  public override void _Ready()
  {
    loading_screen = GD.Load<PackedScene>("res://menus/loading_screen.tscn").Instantiate<Node2D>();
    GetTree().Root.AddChild(loading_screen);
    progress_bar = loading_screen.GetNode<ProgressBar>("AspectRatioContainer/ProgressBar");
    GD.Print("Starting load of " + scene_to_load);
    ResourceLoader.LoadThreadedRequest(scene_to_load, "PackedScene", true);
  }

  // Called every frame. 'delta' is the elapsed time since the previous frame.
  public override void _Process(double delta)
  {

    switch (ResourceLoader.LoadThreadedGetStatus(scene_to_load, progress))
    {
      case ResourceLoader.ThreadLoadStatus.InProgress:
        if (progress_bar != null && progress != null)
          progress_bar.Value = progress[0].As<int>() * 100;
        break;
      case ResourceLoader.ThreadLoadStatus.InvalidResource:
      case ResourceLoader.ThreadLoadStatus.Failed:
        GD.PushError("Couldn't load scene: " + scene_to_load);
        break;
      case ResourceLoader.ThreadLoadStatus.Loaded:
        var scene = ResourceLoader.LoadThreadedGet(scene_to_load);
        if (scene is PackedScene packed)
        {
          GetTree().ChangeSceneToPacked(packed);
          loading_screen.QueueFree();
          QueueFree();
        }
        break;
    }
  }
}
