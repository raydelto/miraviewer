//-----------------------------------------------------------------------------
// basic.frag by Steve Jones 
// Copyright (c) 2015-2019 Game Institute. All Rights Reserved.
//
// Fragment shader for Part 1 of this tutorial
//-----------------------------------------------------------------------------
#version 330 core

in vec2 TexCoord;
out vec4 frag_color;

uniform sampler2D texSampler1;

void main()
{
	frag_color = texture(texSampler1, TexCoord);
}
