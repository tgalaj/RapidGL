#version 460
#define MAX_FRAGMENTS 175

layout (location = 0) out vec4 frag_color;

struct ListNode
{
	vec4 color;
	float depth;
	int coverage;
	uint next;
};

layout (binding = 0, r32ui) uniform uimage2D head_pointers;

layout (binding = 0, std430) buffer linked_lists
{
	ListNode nodes[];
};

void main()
{
	ListNode fragments[MAX_FRAGMENTS];
	int count = 0;

	// Get the index of the head of the list
	uint n = imageLoad(head_pointers, ivec2(gl_FragCoord.xy)).r;

	// Copy the linked list for this fragment into an array
	while (n != 0xffffffff && count < MAX_FRAGMENTS)
	{
		fragments[count] = nodes[n];
		n = fragments[count].next;
		count++;
	}

	// Sort the array by depth using insertion sort (largest to smallest)
	for (uint i = 1; i < count; ++i)
	{
		ListNode to_insert = fragments[i];
		uint j = i;

		while (j > 0 && to_insert.depth > fragments[j - 1].depth)
		{
			fragments[j] = fragments[j - 1];
			--j;
		}
		fragments[j] = to_insert;
	}

	// Traverse the array and combine the colors using the alpha channel
	vec4 color = vec4(0.5, 0.5, 0.5, 1.0); // clear color
	for (uint i = 0; i < count; i++)
	{
		// MSAA support
		if ((fragments[i].coverage & (1 << gl_SampleID)) != 0)
		{
			color.rgb = mix(color.rgb, fragments[i].color.rgb, fragments[i].color.a);
		}
	}

	frag_color = color;
}