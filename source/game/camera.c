#include "game_globals.h"

#define CAM_Z_OFFSET -229376
#define CAM_X_OFFSET 13107
#define CAM_Y_OFFSET 32768
#define CAM_SPEED 131072

static Point cam_target;

void update_cam(uint32 delta_time)
{
    if (camera.world_x < cam_target.pt_X)
    {
        // Move cam right
        camera.world_x += MulSF16(CAM_SPEED, delta_time);

        if (camera.world_x > cam_target.pt_X)
            camera.world_x = cam_target.pt_X;
    }
    else if (camera.world_x > cam_target.pt_X)
    {
        // Move cam left
        camera.world_x -= MulSF16(CAM_SPEED, delta_time);

        if (camera.world_x < cam_target.pt_X)
            camera.world_x = cam_target.pt_X;
    }

    if (camera.world_y < cam_target.pt_Y)
    {
        // Move cam down
        camera.world_y += MulSF16(CAM_SPEED, delta_time);

        if (camera.world_y > cam_target.pt_Y)
            camera.world_y = cam_target.pt_Y;
    }
    else if (camera.world_y > cam_target.pt_Y)
    {
        // Move cam left
        camera.world_y -= MulSF16(CAM_SPEED, delta_time);

        if (camera.world_y < cam_target.pt_Y)
            camera.world_y = cam_target.pt_Y;
    }

    camera.world_z = CAM_BASE_Z_OFFSET + (player.obj->world_z - LEVEL_ZNEAR);
}

void set_cam_target_x(int32 x)
{
     cam_target.pt_X = x;

    // Offset camera for better visuals

    if (cam_target.pt_X > 0)        cam_target.pt_X -= CAM_X_OFFSET;
    else if (cam_target.pt_X < 0)   cam_target.pt_X += CAM_X_OFFSET;
}

void set_cam_target_y(int32 y)
{
    cam_target.pt_Y = y;

    if (LCONTEXT_LEVEL.wrap)
    {
        if (cam_target.pt_Y >= 0)       cam_target.pt_Y -= CAM_Y_OFFSET;
        else if (cam_target.pt_Y < 0)   cam_target.pt_Y += CAM_Y_OFFSET;
    }
    else
    {
        cam_target.pt_Y += CAM_Y_OFFSET;
    }
}

void reset_play_camera(void)
{
    camera.world_x = 0;
    camera.world_y = 0;
    camera.world_z = CAM_BASE_Z_OFFSET;
}

void zero_camera(void)
{
    camera.world_x = 0;
    camera.world_y = 0;
    camera.world_z = 0;
}